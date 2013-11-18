
#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <pthread.h>

typedef struct worker {
    pthread_t thread;
    int terminate;
	int runCount;
    struct workqueue *workqueue;
    struct worker *prev;
    struct worker *next;
} worker_t;

typedef struct job {
    void (*jobFunction)(struct job *job);
    void *userData;
    struct job *prev;
    struct job *next;
} job_t;

typedef struct freejob {
  struct job *freeJobs;
  pthread_mutex_t freeJobsMutex;
} freejob_t;

typedef struct workqueue {
    struct worker *workers;
   ; struct job *waitingJobs;
	struct freejob sfreeJob;
	int freeCount;
	int maxFreeCnt;
	int waitCount;
	int runCountSum;
    pthread_mutex_t jobsMutex;
    pthread_cond_t jobsCond;
} workqueue_t;

class WorkQueue {
 public:
  static int workqueue_init(workqueue_t *aWorkqueue, int aNumWorkers, int aNumFreeJobs);

  static void workqueue_shutdown(workqueue_t *aWorkqueue);

  static void workqueue_add_job(workqueue_t *aWorkqueue, job_t *aJob);

  static void wqFreeJob(workqueue_t *aWorkqueue, job_t *aJob);

  static void wqAllocJob(workqueue_t *aWorkqueue, job_t **aJob);
};

#endif  /* #ifndef WORKQUEUE_H */
