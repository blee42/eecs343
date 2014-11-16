#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

typedef struct pool_t pool_t;


pool_t *pool_create(int thread_count, int queue_size, void* (*routine)(void *));

int pool_add_task(pool_t *pool, void *arg);

int pool_destroy(pool_t *pool);

#endif
