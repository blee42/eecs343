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

#define TRUE 1
#define FALSE 0

// Provided in a original code. Not needed.
// typedef struct {
//     void (*function)(void *);
//     void *argument;
// } pool_task_t;

// Pool has been upgraded to have an array of threads, a function handle
// (since it doesn't make sense to always pass through the function) and
// a queue, implemented as a circular array.
typedef struct pool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t* threads; // array
  void* (*function)(void *); // always handle_connection
  int shutdown;
  int** queue; // circular array of connfds
  int q_start; // circular array start
  int q_end; // circular end
  int thread_count;
  int task_queue_size_limit;
} poolT;

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
    threadpool->shutdown = FALSE;

    threadpool->task_queue_size_limit = queue_size;
    threadpool->queue = (int **) malloc(sizeof(int *) * queue_size);
    threadpool->q_start = 0;
    threadpool->q_end = 0;

    pthread_mutex_init(&threadpool->lock, NULL);
    pthread_cond_init(&threadpool->notify, NULL);

    for(i = 0; i < num_threads; i++)
    {
        pthread_create(&threadpool->threads[i], NULL, thread_do_work, (void *) threadpool);
    }

    return threadpool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void* argument)
{
    pthread_mutex_lock(&pool->lock);

    while ((pool->q_start - 1) % pool->task_queue_size_limit == pool->q_end)
    {
        // do nothing
    }
    pool->queue[pool->q_end] = argument;
    pool->q_end = (pool->q_end + 1) % pool->task_queue_size_limit;

    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    return 0;
}



/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    pool->shutdown = TRUE;

    int i;
    pthread_cond_broadcast(&pool->notify);

    for(i = 0; i < pool->thread_count; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool->threads);
    free(pool->queue);
    free(pool);

    return 0;
}

/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)
{
    poolT* threadpool = (poolT *) pool;

    while (1)
    {
        pthread_mutex_lock(&threadpool->lock);

        while ((threadpool->q_start == threadpool->q_end) && (threadpool->shutdown == FALSE)) {
            pthread_cond_wait(&threadpool->notify, &threadpool->lock);
        }

        if (threadpool->shutdown == TRUE)
            break;

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
