#ifndef HTTP_H
#define HTTP_H
#include<sys/epoll.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
class Http_conn
{
public:
	enum LINE_STATUS{LINE_OK=1,LINE_BAD,LINE_OPEN};//这个是每行的状态
	enum CHECK_STATUS{CHECK_LINE=1,CHECK_HEAD,CHECK_CONTENT};
	enum HTTP_CODE{BAD_RESOURCE=0,UN_FINISH,GET_RESOURCE,FORBIDDEN_RESOURCE,NO_RESOURCE,FILE_RESOURCE,DYNAMIC_RESOURCE};//不是正常的，状态还为完成，解析完成，文件找不到，完全OK
	static const int READSIZE=1024;
	static const int WRITESIZE=1024;
	static int usercout;
	static int  epollfd;
	bool read();
	bool write();//这两个是对网络进行读和写
	void init(int fd,struct sockaddr_in addr1);
	void close();
	void process();//线程池调用的函数全部封装在这里
private:
	
	LINE_STATUS parse_line();//对每一行进行解析
	HTTP_CODE request_line(char *line);//解析请求行
	HTTP_CODE request_head(char*line);//解析请求头
	HTTP_CODE request_content(char*line);
	HTTP_CODE process_read();
	HTTP_CODE do_request();
	void process_write(HTTP_CODE status);//根据读来写
	void clienterror(const char*errnum,const char*shortmsg,const char*longmsg);//用来对错误的填写
	void success_file();//成功时候填写
	bool rio_writen(char *usrbuf,size_t n);//封装的写函数
	void init();
	void dynamic_request();
private:
	char read_buf[READSIZE];
	char head_buf[WRITESIZE];
	char *cgiarg;//用于cgi的参数
	char body_buf[WRITESIZE];
	int read_index;//已经读了多少
	int read_size;//缓冲区实际大小
	int confd;//网络文件描述符
	struct sockaddr_in addr;//客户端地址
	char*method;//所请求的方法只实现get
	bool keepalive;//是否保持连接
	char*url;//url
	char*version;
	int m_start_line;//用来找到解析的每一行的地址
	CHECK_STATUS check_status;
	int contentline;//消息体请求长度
	char *m_host;
	struct stat stat_buf;//用来判断文件类型
	int howsend;//用来判断是否发送文件
};














#endif
