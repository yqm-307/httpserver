/*
    是线程安全的，每步操作都是由mutex解锁、加锁
    block_queue是循环队列

    循环数组
*/


#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "locker.h"
using namespace std;

template <class T>
class block_queue
{
public:
    block_queue(int max_size = 1000);       //构造
    ~block_queue();     //析构  


    void clear();           //清空队列    
    bool full();
    bool empty();

    bool front(T &value) ;  // value赋值为队首元素
    bool back(T &value) ;

    int size();
    int max_size();


    bool push(const T &item);

    //pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item);
    
    //增加了超时处理
    bool pop(T &item, int ms_timeout);

private:
    locker m_mutex;
    cond m_cond;

    T *m_array;
    int m_size;
    int m_max_size;
    int m_front;
    int m_back;
};




/* ——————————————————————————————————实现————————————————————————————————*/

template<typename T>
block_queue<T>::block_queue(int max_size)
 {
    if (max_size <= 0)
    {
        exit(-1);
    }

    m_max_size = max_size;
    m_array = new T[max_size];
    m_size = 0;
    m_front = -1;
    m_back = -1;
}

template<typename T>
void block_queue<T>::clear()
{
    m_mutex.lock();
    m_back=-1;
    m_front=-1;
    m_size=0;
    m_mutex.unlock();
}

template<typename T>
block_queue<T>::~block_queue()
{
    m_mutex.lock();
    if (m_array != NULL)
        delete [] m_array;

    m_mutex.unlock();
}

template<typename T>
bool block_queue<T>::full()
{
    m_mutex.lock();
    if(m_size >= m_max_size)
    {
        m_mutex.unlock();
        return true;
    }
    
    m_mutex.unlock();
    return false;
}

template<typename T>
bool block_queue<T>::empty()
{
    m_mutex.lock();

    if(m_size>0)
    {
        m_mutex.unlock();
        return false;
    }
    
    m_mutex.unlock();
    return true;
}

template<typename T>
bool block_queue<T>::front(T &value)
{
    m_mutex.lock();
    if(0 == m_size)
    {
        m_mutex.unlock();
        return false;   //队列为空
    }

    value = this->m_array[m_front];
    m_mutex.unlock();
    return true;
}

template<typename T>
bool block_queue<T>::back(T& value)
{
    m_mutex.lock();
    if(0==m_size)
    {
        m_mutex.unlock();
        return false;
    }

    value = this->m_array[m_back];
    m_mutex.unlock();
    return true;
}

template<typename T>
int block_queue<T>::size()
{
    int temp = 0;

    m_mutex.lock();
    temp  = m_size;
    m_mutex.unlock();
    
    return temp;
}

template<typename T>
int block_queue<T>::max_size()
{
    int temp=0;
    m_mutex.lock();
    
    temp =  m_max_size;
    m_mutex.unlock();
    return temp;
}

template<typename T>
bool block_queue<T>::push(const T& item)
{
    m_mutex.lock();
    if (m_size >= m_max_size)
    {

        m_cond.broadcast();
        m_mutex.unlock();
        return false;
    }

    m_back = (m_back + 1) % m_max_size;
    m_array[m_back] = item;

    m_size++;

    m_cond.broadcast();
    m_mutex.unlock();
    return true;
}


//往队列添加元素，需要将所有使用队列的线程先唤醒
//当有元素push进队列,相当于生产者生产了一个元素
//若当前没有线程等待条件变量,则唤醒无意义
template<typename T>
bool block_queue<T>::pop(T& item)
{
    m_mutex.lock();
    while (m_size <= 0)
    {
        if (!m_cond.wait(m_mutex))
        {
            m_mutex.unlock();
            return false;
        }
    }
    m_front = (m_front + 1) % m_max_size;
    item = m_array[m_front];
    m_size--;
    m_mutex.unlock();
    return true;
}


template<typename T>
bool block_queue<T>::pop(T& item , int ms_timeout)
{
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    m_mutex.lock();
    if (m_size <= 0)
    {
        t.tv_sec = now.tv_sec + ms_timeout / 1000;
        t.tv_nsec = (ms_timeout % 1000) * 1000;
        if (!m_cond.timewait(m_mutex, t))
        {
            m_mutex.unlock();
            return false;
        }
    }

    if (m_size <= 0)
    {
        m_mutex.unlock();
        return false;
    }

    m_front = (m_front + 1) % m_max_size;
    item = m_array[m_front];
    m_size--;
    m_mutex.unlock();
    return true;
}
#endif
