#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Log
{
public:
    static Log *get_instance()
    {
        static Log* instance = nullptr;
        if(instance == nullptr)
            instance = new Log();
        return instance;
    }
    //io线程
    static void *iothread(void *t)
    {
        Log* p = static_cast<Log*>(t);
        string log;
        while(1)
        {
            if(p->m_log_queue->pop(log))
            {
                p->m_mutex.lock();
                fputs(log.c_str(),p->m_fp);
                p->m_mutex.unlock();
            }
        }
    }

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 1000);

    //写日志
    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    virtual ~Log();
    void *write_log()
    {
        string single_log;
        //从阻塞队列中取出一个日志string，写入文件
        while (m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128]; //路径名
    char log_name[128]; //log文件名
    int m_split_lines;  //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long m_count;  //日志行数记录
    int m_today;        //因为按天分类,记录当前时间是那一天

private:
    FILE *m_fp;         //打开log的文件指针
    char *m_buf;      //公用缓冲区
    block_queue<string> *m_log_queue; //阻塞队列
    locker m_mutex;
};



//预编译，进行简单的包装
#define LOG_DEBUG(format, ...) if(true) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(true) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(true) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(true) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}


#endif






