#ifndef LOCKER_H
#define LOCKER_H

/*
	RAII
*/

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
	sem();
	sem(int num);
	~sem();

	//信号量加一
	bool wait();
	//信号量减一
	bool post();    
private:
	sem_t m_sem;    //信号量
};


class locker
{
public:
	locker();
    ~locker();
    bool lock();
    bool unlock();
    pthread_mutex_t *get();
private:
	pthread_mutex_t m_mutex;
};


class cond
{
public:
	cond();
	~cond();
    	
	
	bool wait(locker&);

	//有时间的等待
    bool timewait(locker&m_mutex, struct timespec t);
	
	bool signal();
    bool broadcast();

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
