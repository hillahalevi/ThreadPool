//
// Created by hilla on 5/6/19.
//

#ifndef __THREAD_POOL__
#define __THREAD_POOL__


#include <sys/types.h>
#include "osqueue.h"


typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

typedef struct {
    void (*function)(void *);
    void *argument;
} Task;

typedef enum {
    threadpool_graceful       = 1
} threadpool_destroy_flags_t;

typedef struct thread_pool {
    pthread_cond_t notify;
    OSQueue* tasks;
    pthread_t *threads;
    pthread_mutex_t  lock;
    pthread_cond_t  threads_all_idle;
    int exitFlag;
    int size;
    int thread_count;
    int started;
    int head;
    int tail;
    int count;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* pool, void (*computeFunc) (void *), void* param);



#endif