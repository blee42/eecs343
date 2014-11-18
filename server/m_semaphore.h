#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

typedef struct m_sem_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
} m_sem_t;

int sem_init(m_sem_t *s);
int sem_destroy(m_sem_t *s);
int sem_wait(m_sem_t *s);
int sem_post(m_sem_t *s);

#endif