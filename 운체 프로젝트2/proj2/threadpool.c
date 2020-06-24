/**
 * Implementation of thread pool.
 */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "threadpool.h"

#define QUEUE_SIZE 10
#define NUMBER_OF_THREADS 3

#define TRUE 1

void exec(void (*somefunction)(void *p), void *p);
// this represents work that has to be 
// completed by a thread in the pool
typedef struct 
{
    void (*function)(void *p);
    void *data;
}
task;
// the worker bee
pthread_t bee[NUMBER_OF_THREADS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t tcount;

task list[QUEUE_SIZE + 1];
size_t liststart = 0, listend = 0;
// insert a task into the queue
// returns 0 if successful or 1 otherwise, 
int enqueue(task t) 
{
    pthread_mutex_lock(&mutex);
    if((listend + 1) % (QUEUE_SIZE + 1) == liststart)
    {
        pthread_mutex_unlock(&mutex);
        return 1;
    }
    list[listend] = t;
    listend = (listend + 1) % (QUEUE_SIZE + 1);
    pthread_mutex_unlock(&mutex);
    return 0;
}

// remove a task from the queue
task dequeue() 
{
    pthread_mutex_lock(&mutex);
    task remove = list[liststart];
    liststart = (liststart + 1) % (QUEUE_SIZE + 1);
    pthread_mutex_unlock(&mutex);
    return remove;
}

// the worker thread in the thread pool
void *worker(void *param)
{
    // execute the task
    task worktodo;
    while(TRUE)
    {
        sem_wait(&tcount);
        worktodo = dequeue();
        exec(worktodo.function, worktodo.data);
    }
    pthread_exit(0);
}

void exec(void (*somefunction)(void *p), void *p)
{
	(*somefunction)(p);
}

/**
 * Submits work to the pool.
 */
int pool_submit(void (*somefunction)(void *p), void *p)
{
    int error = 0;
    task worktodo;
    worktodo.function = somefunction;
    worktodo.data = p;
    error = enqueue(worktodo);
    if(!error)
        sem_post(&tcount);
    return error;
}

// initialize the thread pool
void pool_init(void)
{
    int i = 0;
    sem_init(&tcount, 0, 0);
    while(i < NUMBER_OF_THREADS)
    {
        pthread_create(&bee[i], NULL, worker, NULL);
        i++;
    }
}

// shutdown the thread pool
void pool_shutdown(void)
{
    int k = 0;
    while(k < NUMBER_OF_THREADS)
        pthread_join(bee[k], NULL);

    sem_destroy(&tcount);
    pthread_mutex_destroy(&mutex);
}
