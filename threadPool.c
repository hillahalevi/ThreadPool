//
// Created by hilla on 5/6/19.
//

#include <stddef.h>
#include <malloc.h>
#include <pthread.h>
#include "threadPool.h"


ThreadPool *initialThreads(int numOfThreads, ThreadPool *pool);

ThreadPool *initialThreadPool(int numOfThreads);


static void *threadpool_thread(void *threadpool) {
    ThreadPool *pool = (ThreadPool *) threadpool;
    Task *task;
    for (;;) {
        //lock
        pthread_mutex_lock(&(pool->lock));

        // Wait on condition variable, check for spurious wakeups.
        //  When returning from pthread_cond_wait(), we own the lock. /
        while ((pool->count == 0) && (!pool->exitFlag)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if(osIsQueueEmpty(pool->tasks)){
            break;
        }

        task = (Task *) osDequeue(pool->tasks);
        pool->count--;

        // Unlock */
        pthread_mutex_unlock(&(pool->lock));

        // Get to work /
        (*(task->function))(task->argument);
    }

    pool->started--;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);

}


ThreadPool *initialThreads(int numOfThreads, ThreadPool *pool) {
    // Make room for all the thread
    pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    if (pool->threads == NULL) {
        //TODO handle error
        osDestroyQueue(pool->tasks);
        free(pool);
        return NULL;
    }

    /* Initialize mutex and conditional variable first */
    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0)) {
        tpDestroy(pool, 0);
        return NULL;
    }
    int i;
    /* Start worker threads */
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *) pool) != 0) {
            tpDestroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }
    return pool;


}


ThreadPool *initialThreadPool(int numOfThreads) {
    ThreadPool *pool;
    if ((pool = (ThreadPool *) malloc(sizeof(ThreadPool))) == NULL) {
        //TODO handle error
        return NULL;
    }
    /* Initialize */
    pool->exitFlag = pool->thread_count = pool->head = pool->tail =
    pool->count = pool->started = 0;
    pool->size = numOfThreads;

    //allocate task queue
    pool->tasks = osCreateQueue();
    if (pool->tasks == NULL) {
        //TODO handle error
        free(pool);
        return NULL;
    }

    return initialThreads(numOfThreads, pool);

}


ThreadPool *tpCreate(int numOfThreads) {

    if (numOfThreads <= 0) {
        return NULL;
    }
    return initialThreadPool(numOfThreads);


}


int threadpool_free(ThreadPool *pool) {
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    /* Did we manage to allocate ? */
    if(pool->threads) {
        free(pool->threads);
        osDestroyQueue(pool->tasks);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);
    return 0;
}



void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    int i;
    if(threadPool == NULL) {
        return;
    }
    if(threadPool->exitFlag) {
        return;
    }
    if(pthread_mutex_lock(&(threadPool->lock)) != 0) {
        //TODO HANDLE ERROR
        return ;
    }
    threadPool->exitFlag =1;
    if((pthread_cond_broadcast(&(threadPool->notify)) != 0) ||
    (pthread_mutex_unlock(&(threadPool->lock)) != 0)) {
        //TODO HANDLE ERROR
        return ;
    }
    for(i = 0; i < threadPool->thread_count; i++) {
        if(pthread_join(threadPool->threads[i], NULL) != 0) {
            //TODO HANDLE ERROR
            return ;
        }
    }
    if(shouldWaitForTasks==0){
        while (!osIsQueueEmpty(threadPool->tasks)){
            osDequeue(threadPool->tasks);
        }
    }
    if(shouldWaitForTasks==1){
       // while (!osIsQueueEmpty(threadPool->tasks)){}
    }

    /* Only if everything went well do we deallocate the pool */
    threadpool_free(threadPool);
}

int tpInsertTask(ThreadPool *pool, void (*computeFunc)(void *), void *param) {
    int failed = -1;

    if (pool == NULL || computeFunc == NULL ||pool->exitFlag==1) {
        return failed;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        //TODO HANDLE ERROR
        return failed;
    }

    if (pool->exitFlag) {
        return failed;
    }
    Task *task;
    if ((task = (Task *) malloc(sizeof(Task))) == NULL) {
        //TODO handle error
        return -1;
    }
    task->function = computeFunc;
    task->argument = param;
    osEnqueue(pool->tasks, task);
    pool->count += 1;
    if (pthread_cond_signal(&(pool->notify)) != 0) {
        //TODO handle error
        return failed;
    }

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        //TODO handle error
        return -1;
    }

    return 0;
}

