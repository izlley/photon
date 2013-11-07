
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "workqueue.h"

#define LL_ADD(item, list) { \
    item->prev = NULL; \
    item->next = list; \
    list = item; \
}

#define LL_REMOVE(item, list) { \
    if (item->prev != NULL) item->prev->next = item->next; \
    if (item->next != NULL) item->next->prev = item->prev; \
    if (list == item) list = item->next; \
    item->prev = item->next = NULL; \
}

static void *workerFunction(void *aPtr) {
    worker_t *worker = (worker_t *)aPtr;
    job_t *job;

    while (1) {
        /* Wait until we get notified. */
        pthread_mutex_lock(&worker->workqueue->jobsMutex);
        while (worker->workqueue->waitingJobs == NULL) {
            pthread_cond_wait(&worker->workqueue->jobsCond,
                              &worker->workqueue->jobsMutex);
        }
        job = worker->workqueue->waitingJobs;
        if (job != NULL) {
            LL_REMOVE(job, worker->workqueue->waitingJobs);
        }
        pthread_mutex_unlock(&worker->workqueue->jobsMutex);

        /* If we're supposed to terminate, break out of our continuous loop. */
        if (worker->terminate) break;

        /* If we didn't get a job, then there's nothing to do at this time. */
        if (job == NULL) continue;

        /* Execute the job. */
        job->jobFunction(job);
    }

    free(worker);
    pthread_exit(NULL);
}

int workqueue_init(workqueue_t *aWorkqueue, int aNumWorkers) {
    int i;
    worker_t *worker;
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;

    if (aNumWorkers < 1) aNumWorkers = 1;
    memset(aWorkqueue, 0, sizeof(*aWorkqueue));
    memcpy(&aWorkqueue->jobsMutex, &blank_mutex,
           sizeof(aWorkqueue->jobsMutex));
    memcpy(&aWorkqueue->jobsCond, &blank_cond, sizeof(aWorkqueue->jobsCond));

    for (i = 0; i < aNumWorkers; i++) {
        if ((worker = malloc(sizeof(worker_t))) == NULL) {
            perror("Failed to allocate all workers");
            return 1;
        }
        memset(worker, 0, sizeof(*worker));
        worker->workqueue = workqueue;
        if (pthread_create(&worker->thread, NULL, workerFunction,
                           (void *)worker)) {
            perror("Failed to start all worker threads");
            free(worker);
            return 1;
        }
        LL_ADD(worker, worker->workqueue->workers);
    }

    return 0;
}

void workqueue_shutdown(workqueue_t *aWorkqueue) {
    worker_t *worker = NULL;
	job_t *job = NULL;
	job_t *nxtJob;

    /* Set all workers to terminate. */
    for (worker = aWorkqueue->workers; worker != NULL; worker = worker->next) {
        worker->terminate = 1;
    }

    /* Remove all workers and jobs from the work queue.
     * wake up all workers so that they will terminate. */
    pthread_mutex_lock(&aWorkqueue->jobsMutex);
    aWorkqueue->workers = NULL;
    aWorkqueue->waitingJobs = NULL;
    pthread_cond_broadcast(&aWorkqueue->jobsCond);
    pthread_mutex_unlock(&aWorkqueue->jobsMutex);
	
	pthread_mutex_lock(&aWorkqueue->sfreeJob.freeJobsMutex);
	for (job = aWorkqueue->sfreeJob.freeJobs; job != NULL; job = nxtJob ) {
	  nxtJob = job->next;
      free(job);
	}
	pthread_mutex_unlock(&aWorkqueue->sfreeJob.freeJobsMutex);
}

void workqueue_add_job(workqueue_t *aWorkqueue, job_t *aJob) {
    /* Add the job to the job queue, and notify a worker. */
    pthread_mutex_lock(&aWorkqueue->jobsMutex);
    LL_ADD(aJob, aWorkqueue->waitingJobs);
	++(aWorkqueue->waitCount);
    pthread_cond_signal(&aWorkqueue->jobsCond);
    pthread_mutex_unlock(&aWorkqueue->jobsMutex);
}

void wqFreeJob(workqueue_t *aWorkqueue, job_t *aJob) {
  if (aWorkqueue->freeCount < aWorkqueue->maxFreeCnt) {
	pthread_mutex_lock(&aWorkqueue->sfreeJob.freeJobsMutex);
    LL_ADD(aJob, aWorkqueue->sfreeJob.freeJobs);
    ++(aWorkqueue->freeCount);
    pthread_mutex_unlock(&aWorkqueue->sfreeJob.freeJobsMutex);
  } else {
	free(aJob);
  }
}

void wqAllocJob(workqueue_t *aWorkqueue, job_t *aJob) {
  if (aWorkqueue->freeCount <= 0) {
    if ((aJob = malloc(sizeof(*aJob))) == NULL)
      return;
  } else {
	pthread_mutex_lock(&aWorkqueue->sfreeJob.freeJobsMutex);
	if (aWorkqueue->freeCount > 0) {
	  // assign the first node
	  aJob = aWorkqueue->sfreeJob.freeJobs;
	  LL_REMOVE(aJob, aWorkqueue->sfreeJob.freeJobs);
	  --aWorkqueue->freeCount;
	  pthread_mutex_unlock(&aWorkqueue->sfreeJob.freeJobsMutex);
	} else {
	  pthread_mutex_unlock(&aWorkqueue->sfreeJob.freeJobsMutex);
	  if ((aJob = malloc(sizeof(*aJob))) == NULL)
		return;
	}
  }
}
