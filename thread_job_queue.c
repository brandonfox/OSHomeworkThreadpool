#include<semaphore.h>
#include "threadpool.h"

typedef struct _job_node{
    dispatch_fn function;
    _job_node next;
} _job_node;

_job_node front;
_job_node back;