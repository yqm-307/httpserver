#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "http_conn_message.h"
#include "locker.h"
#include "sql_connection_pool.h"
#include "lst_timer.h"
#include "Util.h"
#include "log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *root, string user, string passwd, string sqlname); //nt sockfd, const sockaddr_in &addr, char *root, string user, string passwd, string sqlname)
                   
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);

    //这两个标志位，决定handle是否完成
    int timer_flag;
    int improv;


private:
    void init();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:

    http_conn_message mg;     //进行业务处理
    int m_sockfd;
    sockaddr_in m_address;
    
    //char *doc_root;

    map<string, string> m_users;


    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
