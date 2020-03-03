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
   int no_threads;
   int alive_threads;
   int freeThreads;
   int killJobs;
   dispatch_fn next_job;
   void *next_job_args;
   pthread_cond_t job_available;
   pthread_cond_t thread_available;
   pthread_cond_t job_taken;
   pthread_mutex_t job_mutex;
} _threadpool;

typedef struct _thread_args {
  _threadpool *pool;
  int id;
} _thread_args;

void doJob(_threadpool *pool,int id){ //ID for debug purposes
  // printf("Thread %d now has a job\n",id);
  dispatch_fn fn = pool->next_job; //Save job
  void *args = pool->next_job_args; //Save args
  pool->freeThreads--; //Mark self as busy
  pthread_cond_signal(&pool->job_taken); //Signal that a job has been taken
  pthread_mutex_unlock(&pool->job_mutex); //Unlock the job mutex
  fn(args);
}

void *worker_thread(void *args) {
  _thread_args* thread = (_thread_args *) args;
  _threadpool *pool = thread->pool;
  int id = thread->id;
  free(thread);
  while (1) {
    pthread_mutex_lock(&pool->job_mutex); //Acquire job lock
    pool->freeThreads++; //Mark self as available
    // printf("Thread %d is now free\n",id);
    //Wait for a job to become available
    pthread_cond_signal(&pool->thread_available); //Signal that a thread is waiting for a job
    pthread_cond_wait(&pool->job_available,&pool->job_mutex);
    if(pool->killJobs){
      // printf("Threadpool is set to die. Committing suicide thread id: %d\n",id);
      pool->alive_threads--;
      pthread_cond_signal(&pool->thread_available);
      pthread_mutex_unlock(&pool->job_mutex);
      return;
    }
    doJob(pool,id);
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
  pool->no_threads = num_threads_in_pool;
  pool->freeThreads = 0;
  pool->killJobs = 0;
  pool->alive_threads = num_threads_in_pool;

  // Init locks
  pthread_mutex_init(&pool->job_mutex,NULL);

  //Init conditions
  pthread_cond_init(&pool->job_available,NULL);
  pthread_cond_init(&pool->thread_available,NULL);
  pthread_cond_init(&pool->job_taken,NULL);

  for(int i = 1; i <= num_threads_in_pool; i++){
    _thread_args *args = malloc(sizeof(_thread_args));
    args->id = i;
    args->pool = pool;
    if(pthread_create((pool->threads + i),NULL,worker_thread,args)){
      fprintf(stderr,"Error creating thread\n");
      exit(1);
    }
  }
  return (threadpool) pool;
}

int get_free_thread(_threadpool* pool){
  for(int i = 0; i < pool->no_threads; i++){
    if(*(pool->threads + i) == 0)
      return i;
  }
  return -1;
}

void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
	      void *arg) {
  _threadpool *pool = (_threadpool *) from_me;
  // printf("Dispatching job..\n");
  pthread_mutex_lock(&pool->job_mutex); //Get job mutex
  // printf("%d threads are free\n",pool->freeThreads);
  if(pool->freeThreads == 0){ //Check to see if a thread is available
    // printf("All threads currently busy\n");
    pthread_cond_wait(&pool->thread_available,&pool->job_mutex); //If no threads are available wait for a thread to finish
  }
  pool->next_job = dispatch_to_here;
  pool->next_job_args = arg;
  // printf("Dispatching job: %d threads are now free\n",pool->freeThreads);
  pthread_cond_signal(&pool->job_available);
  pthread_cond_wait(&pool->job_taken,&pool->job_mutex); //Make sure job is taken before returning
  pthread_mutex_unlock(&pool->job_mutex);
}

void destroy_threadpool(threadpool destroyme) {
  _threadpool *pool = (_threadpool *) destroyme;
  //Make sure each thread terminates
  pthread_mutex_lock(&pool->job_mutex);
  pool->killJobs = 1;
  pthread_cond_broadcast(&pool->job_available);
  while(pool->alive_threads > 0){
    pthread_cond_wait(&pool->thread_available,&pool->job_mutex);
    // printf("A thread has died.\n");
    pthread_cond_signal(&pool->job_available);
  }
  pthread_mutex_unlock(&pool->job_mutex);
  //No need mutexes and conds anymore
  pthread_mutex_destroy(&pool->job_mutex);
  pthread_cond_destroy(&pool->job_available);
  pthread_cond_destroy(&pool->job_taken);
  pthread_cond_destroy(&pool->thread_available);

  //Remove thread references and join threads (Wait for thread to terminate)
  for(int x = 0; x <= pool->no_threads; x++){
    pthread_join(*(pool->threads + x),NULL);
    free(pool->threads + x);
  }
  // add your code here to kill a threadpool
  free(pool);
}
