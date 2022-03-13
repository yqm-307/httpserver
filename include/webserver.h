#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "threadpool.h"
#include "http_conn.h"
//#include "Util.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void init(int port , string user, string passWord, string databaseName,
              int sql_num,int thread_num);

    void thread_pool();     //初始化线程池
    void sql_pool();            //初始化sql连接池
    void log_write();           //初始化log
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclinetdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础
    int m_port;
    char *m_root;
    int m_pipefd[2];
    int m_epollfd;
    http_conn *users;           //http_conn对象数组     大小时MAX_FD，与文件描述符映射

    //数据库相关
    connection_pool *m_connPool;
    string m_user;                             //登陆数据库用户名
    string m_passWord;                  //登陆数据库密码
    string m_databaseName;        //使用数据库名
    int m_sql_num;

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    //内核事务表
    epoll_event events[MAX_EVENT_NUMBER];       //等待处理事件数

    int m_listenfd;     //EventLoop中的监听套接字

    //定时器相关
    client_data *users_timer;       //数组大小是MAX_FD，映射所有connfd
    Utils utils;
};
#endif
