#ifndef MESSAGEPARSE_H
#define MESSAGEPARSE_H

//解析报文、制作报文
class messageparse
{
public:
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
private:
    int bytes_to_send;
    int bytes_have_send;
    CHECK_STATE m_check_state;
    
    char* m_url;            //URL
    char* m_version;    //http版本
    char* m_host;           //主机号


    int m_content_length;
    int m_start_line;       //起始行

    int m_checked_idx;  //检查过的位置下标
    int m_read_idx;         //已读下标
    int m_write_idx;        //已读下标
    int cgi;                          //是否启用post


};

#endif