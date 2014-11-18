#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "m_semaphore.h"

/*
                   SEMAPHORE

The following file contains are implementation
of a semaphore. We've got a slightly renamed
header file, because a semaphore header already
exists. We have methods here to initialize,
destroy, wait, and post. We're statically initalizing
the semaphore count to 1 in here because we're
only really using this semaphore like a mutex.

*/

// Initializes the semaphore, creating all of the
// member components and setting the count to one.
int sem_init(m_sem_t *s)
{	
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->cond, NULL);
	s->count = 1;
	return 0;
}

// Destroys the mutex and cond.
int sem_destroy(m_sem_t *s)
{
    pthread_mutex_destroy(&s->mutex);
    pthread_cond_destroy(&s->cond);
    return 0;
}

// Waits for a signal and decrements
// the semaphore counter.
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

// Broadcasts a signal and increments
// the semaphore counter.
int sem_post(m_sem_t *s)
{
    pthread_mutex_t* mutex = &s->mutex;

    pthread_mutex_lock(mutex);
   	s->count++;
    pthread_cond_broadcast(&s->cond);
    pthread_mutex_unlock(mutex);
    return 0;
}
