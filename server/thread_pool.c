#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "thread_pool.h"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  Feel free to make any modifications you want to the function prototypes and structs
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

#define MAX_THREADS 20
#define STANDBY_SIZE 8

// typedef struct {
//     void (*function)(void *);
//     void *argument;
// } pool_task_t;

typedef struct pool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t *threads; // array
  void* (*function)(void *); // function always handle_connection?
  int** queue; // circular array of connfd
  int q_start; // circular array member
  int q_end; // circular array member
  int thread_count;
  int task_queue_size_limit;
} poolT;

int thread_count=0; // debug counter

static void *thread_do_work(void *pool);


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads, void* (*function)(void *))
{
    poolT* threadpool = (poolT *) malloc(sizeof(poolT));

    int i;
    threadpool->thread_count = num_threads;
    threadpool->threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);

    threadpool->function = function;

    threadpool->task_queue_size_limit = queue_size;
    threadpool->queue = (int **) malloc(sizeof(int *) * queue_size);
    threadpool->q_start = 0;
    threadpool->q_end = 0;

    pthread_mutex_init(&threadpool->lock, NULL); // this is so only one thread changes this struct at at time
    pthread_cond_init(&threadpool->notify, NULL); // so threads waiting to use this struct are reactivated?

    for(i = 0; i < num_threads; i++)
    {
        pthread_create(&threadpool->threads[i], NULL, thread_do_work, (void *) threadpool); // seg fault?
    }

    // TO DO: cond stuff

    return threadpool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void* argument)
{
    pthread_mutex_lock(&pool->lock);

    // add argument to queue in pool struct
    // CHECK if queue full
    pool->queue[pool->q_end] = argument;
    pool->q_end = (pool->q_end + 1) % pool->task_queue_size_limit;
    
    // pthread_t* tid = (pthread_t *) malloc(sizeof(pthread_t));
    // int status = pthread_create(tid, NULL, thread_do_work, (void *) pool);

    printf("created a thread %d\n", thread_count);
    thread_count++;

    pthread_mutex_unlock(&pool->lock);
    return 0;
}



/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    int err = 0;
 
    return err;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)
{
    while (1)
    {
        poolT* threadpool = (poolT *) pool;
        pthread_mutex_lock(&threadpool->lock);

        // + somet stuff from discussion we should implemenet to be thread-safe?
        if (threadpool->q_start != threadpool->q_end)
        {
            int* argument = threadpool->queue[threadpool->q_start];
            threadpool->q_start = (threadpool->q_start + 1) % threadpool->task_queue_size_limit;
            threadpool->function(argument);
        }

        pthread_mutex_unlock(&threadpool->lock);
    }

    pthread_exit(NULL);
    return(NULL);
}
