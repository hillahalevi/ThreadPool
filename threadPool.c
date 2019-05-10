//
//
//

#include <stddef.h>
#include <malloc.h>
#include <pthread.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include "threadPool.h"

#define PRINT_ERROR_EXIT write(2, "Error in system call\n", strlen("Error in system call\n")); exit(-1);

ThreadPool *initialThreads(int numOfThreads, ThreadPool *pool);

ThreadPool *initialThreadPool(int numOfThreads);


static void *thread_run(void *threadPool) {
    ThreadPool *pool = (ThreadPool *) threadPool;
    Task *task;
    while(1) {
        //lock
        pthread_mutex_lock(&(pool->lock));

        // Wait on condition variable
        while ((pool->count == 0) && (!pool->exitFlag)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if(osIsQueueEmpty(pool->tasks)){
            //task is empty - break
            break;
        }

        task = (Task *) osDequeue(pool->tasks);
        pool->count--;

        // Unlock
        pthread_mutex_unlock(&(pool->lock));

        // Get to work
        (*(task->function))(task->argument);
        free(task);
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
        PRINT_ERROR_EXIT
    }

    /* Initialize mutex and conditional variable first */
    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0)) {
        tpDestroy(pool, 0);
        PRINT_ERROR_EXIT
    }
    int i;
    /* Start worker threads */
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, thread_run, (void *) pool) != 0) {
            tpDestroy(pool, 0);
            PRINT_ERROR_EXIT
        }
        pool->threadCount++;
        pool->started++;
    }
    return pool;


}


ThreadPool *initialThreadPool(int numOfThreads) {
    ThreadPool *pool;
    if ((pool = (ThreadPool *) malloc(sizeof(ThreadPool))) == NULL) {
        //TODO handle error
        PRINT_ERROR_EXIT
    }
    /* Initialize */
    pool->exitFlag = pool->threadCount = pool->count = pool->started = 0;

    //allocate task queue
    pool->tasks = osCreateQueue();
    if (pool->tasks == NULL) {
        //TODO handle error
        free(pool);
        PRINT_ERROR_EXIT
    }

    return initialThreads(numOfThreads, pool);

}


ThreadPool *tpCreate(int numOfThreads) {

    if (numOfThreads <= 0) {
        return NULL;
    }
    return initialThreadPool(numOfThreads);


}


int threadPoolFree(ThreadPool *pool) {
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
    if(threadPool == NULL || threadPool->exitFlag) {
        return;
    }
    if(pthread_mutex_lock(&(threadPool->lock)) != 0) {
        PRINT_ERROR_EXIT
        //TODO HANDLE ERROR
    }
    threadPool->exitFlag =1;
    if((pthread_cond_broadcast(&(threadPool->notify)) != 0) ||
    (pthread_mutex_unlock(&(threadPool->lock)) != 0)) {
        //TODO HANDLE ERROR
        PRINT_ERROR_EXIT
    }
    for(i = 0; i < threadPool->threadCount; i++) {
        if(pthread_join(threadPool->threads[i], NULL) != 0) {
            //TODO HANDLE ERROR
            PRINT_ERROR_EXIT
        }
    }
    if(shouldWaitForTasks==0){
        while (!osIsQueueEmpty(threadPool->tasks)){
            osDequeue(threadPool->tasks);
        }
    } else{
        while (!osIsQueueEmpty(threadPool->tasks)){}
    }

    //now were ready to free the pool
    threadPoolFree(threadPool);
}


int tpInsertTask(ThreadPool *pool, void (*computeFunc)(void *), void *param) {
    int failed = -1;

    if (pool == NULL || computeFunc == NULL ||pool->exitFlag==1) {
        return failed;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        //TODO HANDLE ERROR
        tpDestroy(pool,0);
        PRINT_ERROR_EXIT
    }
    Task *task;
    if ((task = (Task *)malloc(sizeof(Task))) == NULL) {
        //TODO handle error
        tpDestroy(pool,0);
        PRINT_ERROR_EXIT
    }
    task->function = computeFunc;
    task->argument = param;
    osEnqueue(pool->tasks, task);
    pool->count += 1;
    if ((pthread_cond_signal(&(pool->notify)) != 0)||(pthread_mutex_unlock(&pool->lock) != 0)) {
        //TODO handle error
        tpDestroy(pool,0);
        PRINT_ERROR_EXIT
    }


    return 0;
}

