#include "../include/http_conn.h"

#include <mysql/mysql.h>
#include <fstream>

locker m_lock;
map<string, string> users;

//初始化sql连接池
void http_conn::initmysql_result(connection_pool *connPool)
{
    //先从连接池中取一个连接
    MYSQL *mysql = NULL;
    connectionRAII mysqlcon(&mysql, connPool);

    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

//对文件描述符设置非阻塞
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;


    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;


    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//从内核时间表删除描述符
void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//将事件重置为   ( EPOLLONESHOT | ev )
void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;

    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}


//这两个是static 的 ， 所以使用时需要加锁
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;


/*  
*   关闭一个客户的socket 连接
*   一个connfd生命期到此结束
*/
void http_conn::close_conn(bool real_close)
{
    if (real_close && (m_sockfd != -1))
    {
        printf("close %d\n", m_sockfd);     //关闭客户连接，输出到屏幕
        removefd(m_epollfd, m_sockfd);      //从epoll_event
        m_sockfd = -1;
        m_user_count--;
    }
}

/*  
*   初始化一个连接  
*   将sockfd、addr、sql数据初始化为一个 http_conn 对象
*/
void http_conn::init(int sockfd, const sockaddr_in &addr, char *root,
                    string user, string passwd, string sqlname)
{
    m_sockfd = sockfd;
    m_address = addr;

    addfd(m_epollfd, sockfd, true);
    m_user_count++;

    //当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
    mg.doc_root = root;

    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());

    init();
    //m_mg.init();
}

//初始化新接受的连接
//check_state默认为分析请求行状态
/*
*   初始化
*   这里的关联都是涉及一个报文处理，想把它抽象出来
*/
void http_conn::init()
{
    mysql = NULL;
    m_state = 0;
    //决定程序是否正确退出
    timer_flag = 0;
    improv = 0;

    mg.init();
}


//循环读取客户数据，直到无数据可读或对方关闭连接
//非阻塞ET工作模式下，需要一次性将数据读完
bool http_conn::read_once()
{
    if (mg.m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;


    //ET读数据
    while (true)
    {
        bytes_read = recv(m_sockfd, mg.m_read_buf + mg.m_read_idx, READ_BUFFER_SIZE - mg.m_read_idx, 0);
        if (bytes_read == -1)
        {
            if (errno == EAGAIN)    //
                break;
            return false;
        }
        else if (bytes_read == 0)
        {
            return false;
        }
        mg.m_read_idx += bytes_read;
    }
    return true;
}






//
bool http_conn::write()
{
    int temp = 0;

    if (mg.bytes_to_send == 0)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);    //将sockfd在 epoll_event 中的关注事件改为 EPOLLIN
        init();
        return true;
    }

    while (1)
    {
        temp = writev(m_sockfd, mg.m_iv, mg.m_iv_count);  //核心，真正做事儿的

        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            mg.unmap();
            return false;
        }

        mg.bytes_have_send += temp;
        mg.bytes_to_send -= temp;
        if (mg.bytes_have_send >= mg.m_iv[0].iov_len)
        {
            mg.m_iv[0].iov_len = 0;
            mg.m_iv[1].iov_base = mg.m_file_address + (mg.bytes_have_send - mg.m_write_idx);
            mg.m_iv[1].iov_len = mg.bytes_to_send;
        }
        else
        {
            mg.m_iv[0].iov_base = mg.m_write_buf + mg.bytes_have_send;
            mg.m_iv[0].iov_len = mg.m_iv[0].iov_len - mg.bytes_have_send;
        }

        if (mg.bytes_to_send <= 0)
        {
            mg.unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN);

            if (mg.m_linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}


void http_conn::process()
{
    http_conn_message::HTTP_CODE read_ret = mg.process_read(users,m_lock,mysql);
    if (read_ret == http_conn_message::NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    bool write_ret = mg.process_write(read_ret);
    if (!write_ret)
    {
        close_conn();   //关闭连接，并打印关闭信息
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}
