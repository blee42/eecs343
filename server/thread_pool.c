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
  pool_task_t *queue; // array of connfd
  int thread_count;
  int task_queue_size_limit;
} poolT;

int thread_count=0;

static void *thread_do_work(void *pool);


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads, void* (*function)(void *))
{
    // poolT* threadpool = malloc(sizeof(poolT));
    // threadpool->thread_count = num_threads;
    // threadpool->task_queue_size_limit = queue_size
    // threadpool->threads = malloc(sizeof(pthread_t) * num_threads);

    return NULL;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void *argument)
{
    // add argument to queue in pool struct
    
    int status;
    pthread_t* tid = (pthread_t *) malloc(sizeof(pthread_t));
    status = pthread_create(tid, NULL, thread_do_work, (void *) pool);

    printf("created a thread %d\n", thread_count);
    thread_count++;



    return status;
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

    while(1) {
      // extract function and arg
      // call function

      // look at discussion slides
        
    }

    pthread_exit(NULL);
    return(NULL);
}
