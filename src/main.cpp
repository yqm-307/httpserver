#include "../include/config.h"


void config(WebServer&);
//主函数
int main(int argc, char *argv[])
{
    WebServer server;   
    config(server);

    //EventLoop
    server.eventLoop();

    return 0;
}











void config(WebServer &server)
{   
    //需要修改的数据库信息,登录名,密码,库名
    string user = "yqmsql";
    string passwd = "200101";
    string databasename = "yqmserver";

    Config config;


    //初始化
    server.init(config.PORT, user, passwd, databasename,
                config.sql_num, config.thread_num);
    

    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();

    //监听
    server.eventListen();
}