/**
 * threadpool.c
 *
 * This file will contain your implementation of a threadpool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "threadpool.h"

// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers
typedef struct _threadpool_st {
   // you should fill in this structure with whatever you need
   pthread_t *threads;
   int noThreads;
   int freeThreads;
   int jobPending;
   dispatch_fn nextJob;
   void *nextJobArgs;
   pthread_cond_t jobAvailable;
   pthread_cond_t threadAvailable;
   pthread_cond_t jobTaken;
   pthread_mutex_t jobMutex;
} _threadpool;

typedef struct _thread_args {
  _threadpool *pool;
  int id;
} _thread_args;

void doJob(_threadpool *pool,int id){ //ID for debug purposes
  // printf("Thread %d now has a job\n",id);
  dispatch_fn fn = pool->nextJob; //Save job
  void *args = pool->nextJobArgs; //Save args
  pool->jobPending = 0; //Mark job as taken
  pool->freeThreads--; //Mark self as busy
  pthread_cond_signal(&pool->jobTaken); //Signal that a job has been taken
  pthread_mutex_unlock(&pool->jobMutex); //Unlock the job mutex
  fn(args);
}

void *worker_thread(void *args) {
  _thread_args* thread = (_thread_args *) args;
  _threadpool *pool = thread->pool;
  int id = thread->id;
  free(thread);
  while (1) {
    pthread_mutex_lock(&pool->jobMutex); //Acquire job lock
    pool->freeThreads++; //Mark self as available
    // printf("Thread %d is now free\n",id);
    if(pool->jobPending == 1){ //If there is a pending job and this thread is the mutex lock holder
      doJob(pool,id);
    }
    else{
      //Wait for a job to become available
      pthread_cond_signal(&pool->threadAvailable); //Signal that a thread is waiting for a job
      pthread_cond_wait(&pool->jobAvailable,&pool->jobMutex);
      doJob(pool,id);
    }
  }
}


threadpool create_threadpool(int num_threads_in_pool) {
  _threadpool *pool;

  // sanity check the argument
  if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
    return NULL;

  pool = (_threadpool *) malloc(sizeof(_threadpool));
  if (pool == NULL) {
    fprintf(stderr, "Out of memory creating a new threadpool!\n");
    return NULL;
  }
  // Init data
  pool->threads = malloc(sizeof(pthread_t) * num_threads_in_pool);
  pool->noThreads = num_threads_in_pool;
  pool->freeThreads = 0;
  pool->jobPending = 0;

  // Init locks
  pthread_mutex_init(&pool->jobMutex,NULL);

  //Init conditions
  pthread_cond_init(&pool->jobAvailable,NULL);
  pthread_cond_init(&pool->threadAvailable,NULL);
  pthread_cond_init(&pool->jobTaken,NULL);

  for(int i = 1; i <= num_threads_in_pool; i++){
    _thread_args *args = malloc(sizeof(_thread_args));
    args->id = i;
    args->pool = pool;
    if(pthread_create((pool->threads + i * sizeof(pthread_t)),NULL,worker_thread,args)){
      fprintf(stderr,"Error creating thread\n");
      exit(1);
    }
  }
  return (threadpool) pool;
}

int getFreeThread(_threadpool* pool){
  for(int i = 0; i < pool->noThreads; i++){
    if(*(pool->threads + sizeof(pthread_t) * i) == 0)
      return i;
  }
  return -1;
}

void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
	      void *arg) {
  _threadpool *pool = (_threadpool *) from_me;
  // printf("Dispatching job..\n");
  pthread_mutex_lock(&pool->jobMutex); //Get job mutex
  // printf("%d threads are free\n",pool->freeThreads);
  if(pool->jobPending){
    // printf("Thread hasnt taken job yet\n");
    pthread_cond_wait(&pool->jobTaken,&pool->jobMutex);
  }
  if(pool->freeThreads == 0){ //Check to see if a thread is available
    // printf("All threads currently busy\n");
    pthread_cond_wait(&pool->threadAvailable,&pool->jobMutex); //If no threads are available wait for a thread to finish
  }
  pool->nextJob = dispatch_to_here;
  pool->nextJobArgs = arg;
  pool->jobPending = 1;
  // printf("Dispatching job: %d threads are now free\n",pool->freeThreads);
  pthread_cond_signal(&pool->jobAvailable);
  pthread_mutex_unlock(&pool->jobMutex);
}

void destroy_threadpool(threadpool destroyme) {
  _threadpool *pool = (_threadpool *) destroyme;
  //Remove thread references and join threads (Wait for thread to terminate)
  for(int x = 0; x < pool->noThreads; x++){
    pthread_join(*(pool->threads + sizeof(pthread_t) * x),NULL);
    free(pool->threads + sizeof(pthread_t) * x);
  }
  // add your code here to kill a threadpool
}
