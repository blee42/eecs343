#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "m_semaphore.h"

int sem_init(m_sem_t *s)
{	
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->cond, NULL);
	s->count = 1;
	return 0;
}

int sem_wait(m_sem_t *s)
{
	pthread_mutex_t* mutex = &s->mutex;

    pthread_mutex_lock(mutex);
    while (s->count <= 0)
    {
    	pthread_cond_wait(&s->cond, mutex);
    }
    s->count--;
    pthread_mutex_unlock(mutex);
    return 0;
}

int sem_post(m_sem_t *s)
{
    pthread_mutex_t* mutex = &s->mutex;

    pthread_mutex_lock(mutex);
   	s->count++;
    pthread_cond_broadcast(&s->cond);
    pthread_mutex_unlock(mutex);
    return 0;
}
