/**
 * Multithreaded, evhttp-based http server.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <signal.h>

#include "workqueue.h"
#include "exec-get.h"
#include "exec-post.h"
#include "hbase-op.h"

/* Port to listen on. */
#define SERVER_PORT 8080
/* Connection backlog (# of backlogged connections to accept). */
#define CONNECTION_BACKLOG 8
/* Number of worker threads.  Should match number of CPU cores reported in 
 * /proc/cpuinfo. */
#define NUM_THREADS 8

#define NUM_FREE_JOB 100

#define HBASE_ADDR "127.0.0.1"
#define HBASE_PORT 9090
#define HBASE_CREDIT_TBL "credit"

/* Behaves similarly to fprintf(stderr, ...), but adds file, line, and function
 information. */
#define errorOut(...) {\
    fprintf(stderr, "%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
}

/**
 * Struct to carry around connection (client)-specific data.
 */
typedef struct client {
    /* The event_base for this client. */
    struct event_base *evbase;

    /* The evhttp for this client. */
    struct evhttp *evhttp;

    /* The evhttp_bound_socket for this client. */
    struct evhttp_bound_socket *handle;
} client_t;

static struct event_base *evbase_accept;
static workqueue_t workqueue;
unsigned long count = 0;

/* Signal handler function (defined below). */
static void sighandler(int signal);

static void closeClient(client_t *client) {
    if (client != NULL) {
        if (client->handle != NULL) {
            evutil_socket_t client_fd
                = evhttp_bound_socket_get_fd(client->handle);
            if (client_fd > 0) {
                evutil_closesocket(client_fd);
            }
            evhttp_del_accept_socket(client->evhttp, client->handle);
            free(client->handle);
            client->handle = NULL;
        }
    }
}

static void closeAndFreeClient(client_t *client) {
    if (client != NULL) {
        closeClient(client);
        if (client->evhttp != NULL) {
            evhttp_free(client->evhttp);
            client->evhttp = NULL;
        }
        if (client->evbase != NULL) {
            event_base_free(client->evbase);
            client->evbase = NULL;
        }
        free(client);
    }
}

/**
 * Called by libevent when there is request coming.
 */
void request_on_receivd(struct evhttp_request *req, void *arg) {
  struct evbuffer *sEvb = NULL;
  struct evkeyvalq *sKv = NULL;
  client_t *client = (client_t *)arg;
  const char *cmdtype;
  struct evhttp_uri *sDecoded = NULL;
  const char  *sPath;
  char *sDecodedPath;
  char *sWholePath = NULL;
  
  const char *sUri = evhttp_request_get_uri(req);
  sDecoded = evhttp_uri_parse(sUri);

  if (!sDecoded) {
    printf("It's not a good URI. Sending BADREQUEST\n");
    evhttp_send_error(req, HTTP_BADREQUEST, 0);
    return;
  }
  
  /* Let's see what path the user asked for. */
  sPath = evhttp_uri_get_path(sDecoded);
  if (!sPath) sPath = "/";
  
  sDecodedPath = evhttp_uridecode(sPath, 0, NULL);
  if (sDecodedPath == NULL)
    goto err;

  if (strstr(sDecodedPath, ".."))
    goto err;

  
  //sInHead = evhttp_request_get_input_headers(req);
  //int evhttp_parse_query_str(sUri, sKv);
  
  printf("evhttp_uri_get_path : %s\n evhttp_uridecode : %s\n", sPath, sDecodedPath);
  
  sEvb = evbuffer_new();
  
  switch(evhttp_request_get_command(req)) {
	case EVHTTP_REQ_GET: 
	  cmdtype = "GET";
	  if (ExecGet::execGet(evhttp_request_get_uri(req), sEvb) ==
          FAILURE )
        goto err;
	  break;
    case EVHTTP_REQ_POST:
	  cmdtype = "POST";
	  break;
    case EVHTTP_REQ_HEAD:
	  cmdtype = "HEAD";
	  break;
    case EVHTTP_REQ_PUT:
	  cmdtype = "PUT";
	  break;
    case EVHTTP_REQ_DELETE:
	  cmdtype = "DELETE";
	  break;
    case EVHTTP_REQ_OPTIONS:
	  cmdtype = "OPTIONS";
	  break;
    case EVHTTP_REQ_TRACE:
	  cmdtype = "TRACE";
	  break;
    case EVHTTP_REQ_CONNECT:
	  cmdtype = "CONNECT";
	  break;
    case EVHTTP_REQ_PATCH:
	  cmdtype = "PATCH";
	  break;
    default:
	  cmdtype = "unknown";
	  break;
  }

  if (++count % 100 == 0) {
    printf("Count : %lu, Received a %s request for %s\n",
    ++count, cmdtype, evhttp_request_get_uri(req));
  }

  /* Send a 200 OK reply to client */
  evhttp_send_reply(req, 200, "OK", sEvb);
  return;
err:
  evhttp_send_error(req, HTTP_NOTFOUND, "Document was not found");
  printf("ERROR\n");
done:
  if (sDecoded)
    evhttp_uri_free(sDecoded);
  if (sDecodedPath)
    free(sDecodedPath);
  if (sWholePath)
    free(sWholePath);
  if (sEvb)
    evbuffer_free(sEvb);
}



static void server_job_function(struct job *aJob) {
    client_t *client = (client_t *)aJob->userData;

    event_base_dispatch(client->evbase);
    closeAndFreeClient(client);
    WorkQueue::wqFreeJob(&workqueue, aJob);
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
void on_accept(evutil_socket_t fd, short ev, void *arg) {
    workqueue_t *sWorkqueue = (workqueue_t *)arg;
    client_t *client;
    job_t *job;

    /* Create a client object. */
    if ((client = (client_t *)malloc(sizeof(*client))) == NULL) {
        warn("failed to allocate memory for client state");
        return;
    }
    memset(client, 0, sizeof(*client));

    /* Create new event_base, evhttp and accept a new connection for client. */
    if ((client->evbase = event_base_new()) == NULL) {
        warn("client event_base creation failed");
        closeAndFreeClient(client);
        return;
    }
    if ((client->evhttp = evhttp_new(client->evbase)) == NULL) {
        warn("client evhttp creation failed");
        closeAndFreeClient(client);
        return;
    }
    client->handle = evhttp_accept_socket_with_handle(client->evhttp, fd);
    if (client->handle == NULL) {
        warn("accept failed");
        closeAndFreeClient(client);
        return;
    }

    /* Set the client socket to non-blocking mode. */
    evutil_socket_t client_fd = evhttp_bound_socket_get_fd(client->handle);
    if (evutil_make_socket_nonblocking(client_fd) < 0) {
        warn("failed to set client socket to non-blocking");
        closeAndFreeClient(client);
        return;
    }

    /* Add any custom code anywhere from here to the end of this function
     * to initialize your application-specific attributes in the client struct.
     */

    /* Set the default callback for evhttp */
    evhttp_set_gencb(client->evhttp, request_on_receivd, client);

    /* Create a job object and add it to the work queue. */
	(void)WorkQueue::wqAllocJob(sWorkqueue, &job);
    if (job == NULL) {
        warn("failed to allocate memory for job state");
        closeAndFreeClient(client);
        return;
    }
    job->jobFunction = server_job_function;
    job->userData = client;

    WorkQueue::workqueue_add_job(sWorkqueue, job);
}

/**
 * Run the server.  This function blocks, only returning when the server has 
 * terminated.
 */
int runServer(void) {
    evutil_socket_t listenfd;
    struct sockaddr_in listen_addr;
    struct event *ev_accept;
    int reuseaddr_on;

    /* Set signal handlers */
    sigset_t sigset;
    sigemptyset(&sigset);
    struct sigaction siginfo;
	memset(&siginfo, 0x00, sizeof(siginfo));
	siginfo.sa_handler = sighandler;
	siginfo.sa_mask = sigset;
	siginfo.sa_flags = SA_RESTART;
    sigaction(SIGINT, &siginfo, NULL);
    sigaction(SIGTERM, &siginfo, NULL);

    /* Create our listening socket. */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(1, "listen failed");
    }

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(SERVER_PORT);
    if (bind(listenfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        err(1, "bind failed");
    }
    if (listen(listenfd, CONNECTION_BACKLOG) < 0) {
        err(1, "listen failed");
    }
    reuseaddr_on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
               sizeof(reuseaddr_on));

    /* Set the socket to non-blocking, this is essential in event
     * based programming with libevent. */
    if (evutil_make_socket_nonblocking(listenfd) < 0) {
        err(1, "failed to set server socket to non-blocking");
    }

    if ((evbase_accept = event_base_new()) == NULL) {
        perror("Unable to create socket accept event base");
        close(listenfd);
        return 1;
    }

    /* Initialize work queue. */
    if (WorkQueue::workqueue_init(&workqueue, NUM_THREADS, NUM_FREE_JOB)) {
        perror("Failed to create work queue");
        close(listenfd);
        WorkQueue::workqueue_shutdown(&workqueue);
        return 1;
    }

    /* We now have a listening socket, we create a read event to
     * be notified when a client connects. */
    ev_accept = event_new(evbase_accept, listenfd, EV_READ|EV_PERSIST,
                          on_accept, (void *)&workqueue);
    event_add(ev_accept, NULL);

    printf("Server running.\n");
    
    // conf for hbase
    HBaseOp::setHostAddr((char*)HBASE_ADDR);
    HBaseOp::setHostPort(HBASE_PORT);
    HBaseOp::setCreditTbl(HBASE_CREDIT_TBL);

    /* Start the event loop. */
    event_base_dispatch(evbase_accept);

    event_base_free(evbase_accept);
    evbase_accept = NULL;

    close(listenfd);

    printf("Server shutdown.\n");

    return 0;
}

/**
 * Kill the server.  This function can be called from another thread to kill
 * the server, causing runServer() to return.
 */
void killServer(void) {
    fprintf(stdout, "Stopping socket listener event loop.\n");
    if (event_base_loopexit(evbase_accept, NULL)) {
        perror("Error shutting down server");
    }
    fprintf(stdout, "Stopping workers.\n");
    WorkQueue::workqueue_shutdown(&workqueue);
}

static void sighandler(int signal) {
    fprintf(stdout, "Received signal %d: %s.  Shutting down.\n", signal,
            strsignal(signal));
    killServer();
}

/* Main function for demonstrating the echo server.
 * You can remove this and simply call runServer() from your application. */
int main(int argc, char *argv[]) {
    return runServer();
}
