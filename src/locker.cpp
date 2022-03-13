#include "../include/locker.h"

//构造函数
sem::sem()
{
    sem_init(&m_sem,0,0);
}
sem::sem(int num)
{
    sem_init(&m_sem,0,num);
}
sem::~sem()
{
    sem_destroy(&m_sem);
}
bool sem::wait()
{
    return sem_wait(&m_sem)==0;
}
bool sem::post()
{
    return sem_post(&m_sem)==0;
}




locker::locker()
{
    pthread_mutex_init(&m_mutex,NULL);
}
locker::~locker()
{
    pthread_mutex_destroy(&m_mutex);
}
bool locker::lock()
{
    return pthread_mutex_lock(&m_mutex)==0;
}
bool locker::unlock()
{
    return pthread_mutex_unlock(&m_mutex);
}
pthread_mutex_t* locker::get()
{
    return &m_mutex;
}




cond::cond()
{
    pthread_cond_init(&m_cond,NULL);
}
cond::~cond()
{
    pthread_cond_destroy(&m_cond);
}
bool cond::wait(locker& mutex)
{
    return pthread_cond_wait(&m_cond,mutex.get());    
}
bool cond::timewait(locker& mutex,struct timespec t)
{
    return pthread_cond_timedwait(&m_cond,mutex.get(),&t);
}
bool cond::signal()
{
    return pthread_cond_signal(&m_cond);
}
bool cond::broadcast()
{
    return pthread_cond_broadcast(&m_cond);
}
