//
// Created by hilla on 5/6/19.
//

#ifndef __THREAD_POOL__
#define __THREAD_POOL__


#include <sys/types.h>
#include "osqueue.h"



typedef struct {
    void (*function)(void *);
    void *argument;
} Task;


typedef struct thread_pool {
    pthread_cond_t notify;
    OSQueue* tasks;
    pthread_t *threads;
    pthread_mutex_t  lock;
    int exitFlag;
    int threadCount;
    int started;
    int count;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* pool, void (*computeFunc) (void *), void* param);



#endif