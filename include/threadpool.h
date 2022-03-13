#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <stdlib.h>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "sql_connection_pool.h"

template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    static void *worker(void *arg); //work调用run
    void run();                     //实际工作

private:
    int m_thread_number;         //线程池中的线程数
    int m_max_requests;          //请求队列中允许的最大请求数
    pthread_t *m_threads;        //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue;  //请求队列
    locker m_queuelocker;        //保护请求队列的互斥锁
    sem m_queuestat;             //是否有任务需要处理
    connection_pool *m_connPool; //数据库连接池
};

/*—————————————————————————————————————————————实现—————————————————————————————————————————————————————*/

//构造
template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests)
    : //m_actor_model(actor_model),
      m_thread_number(thread_number),
      m_max_requests(max_requests),
      m_threads(NULL),
      m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
	exit(-1);
    m_threads = new pthread_t[m_thread_number]; //创建MAX 数量的 句柄数组
    if (!m_threads)
	exit(-1);

    //创建 MAX 数量的线程
    for (int i = 0; i < thread_number; ++i)
    {
	if (pthread_create(m_threads + i, NULL, worker, this) != 0) //工作线程
	{
	    delete[] m_threads;
	    exit(-1);
	}
	if (pthread_detach(m_threads[i]))
	{
	    delete[] m_threads;
	    exit(-1);
	}
    }
}

//删除所有句柄
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}

/*插入一个请求*/
template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
	m_queuelocker.unlock();
	return false; //没有空闲线程就失败
    }
    request->m_state = state;
    m_workqueue.push_back(request); //把事件放在队列中排队
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

/*插入一个请求*/
template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
	m_queuelocker.unlock();
	return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

/*
    Reactor模式中的  handle ，在主循环（EventLoop）中分发事件给handle，handle根据事件信息进行处理
*/
template <typename T>
void threadpool<T>::run()
{
    //这个循环中不断从请求队列中，取出事件处理
    while (true)
    {
	m_queuestat.wait();
	m_queuelocker.lock();
	if (m_workqueue.empty())
	{
	    m_queuelocker.unlock();
	    continue;
	}
	T *request = m_workqueue.front();
	m_workqueue.pop_front();
	m_queuelocker.unlock();
	if (!request)
	    continue;

	if (0 == request->m_state)
	{
	    if (request->read_once())
	    {
		request->improv = 1;
		connectionRAII mysqlcon(&request->mysql, m_connPool);
		request->process();
	    }
	    else
	    {
		request->improv = 1;
		request->timer_flag = 1;
	    }
	}
	else
	{
	    if (request->write())
	    {
		request->improv = 1;
	    }
	    else
	    {
		request->improv = 1;
		request->timer_flag = 1;
	    }
	}
    }
}
#endif
