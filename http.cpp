#include"http.h"
#include<string.h>
#include<sys/stat.h>
#include<cerrno>
#include<cstdio>
#include<sys/mman.h>
#include<stdlib.h>
#include<sys/wait.h>
int Http_conn::epollfd=-1;
int Http_conn::usercout=0;
void setnoblocking(int fd)//ET妯″紡閰嶅悎闈為樆濉?{
	int old=fcntl(fd,F_GETFL);
	int newoption=old|O_NONBLOCK;
	fcntl(fd,F_SETFL,newoption);
}
void addfd(int epollfd ,int fd,bool oneshot)//璁剧疆bool鐨勭洰鐨勬槸鍥犱负鐩戝惉鎻忚堪绗︿笉鐢╫neshot
{
        struct epoll_event ev;
        ev.data.fd=fd;
        ev.events=EPOLLIN|EPOLLET|EPOLLHUP;//ET妯″紡
	if(oneshot)
	{
		ev.events|EPOLLONESHOT;
	}
        epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
	setnoblocking(fd);
}
void modfd(int epollfd,int fd,int event)//涓€寮€濮嬫垜浠彧鐩戝惉璇荤劧鍚庣幇鍦ㄤ慨鏀逛负鐩戝惉鍐?{
	struct epoll_event ev;
	ev.data.fd=fd;
	ev.events=EPOLLET|EPOLLHUP|event|EPOLLONESHOT;//
	epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}
bool Http_conn::read()
{
	char *buf=read_buf;
	int count;
	while(read_size<=READSIZE)
	{
		count=::read(confd,buf,READSIZE);
		if(count<0)
		{
			if(errno==EAGAIN)//浠ｈ〃宸茬粡娌℃湁鏁版嵁浜?				return true;
			else if(errno==EINTR)//浠ｈ〃琚俊鍙蜂腑鏂?			{
				count=0;
				continue;
			}
			else 
				return false;
		}
		else if(count==0)//瀵规柟鍏抽棴杩炴帴
			return false;
		read_size+=count;
		buf+=count;
	}
	return false;
}
Http_conn::LINE_STATUS Http_conn:: parse_line()
{
	while(read_index<read_size)
	{
		if(read_buf[read_index]=='\r')
		{
			if(read_index+1==read_size)
				return LINE_OPEN;
			if(read_buf[read_index+1]=='\n')
			{
				read_buf[read_index++]='\0';
				read_buf[read_index++]='\0';
				return LINE_OK;
			}
			else
				return LINE_BAD;
		}
		read_index++;
	}
	return LINE_OPEN;
}
Http_conn::HTTP_CODE Http_conn::request_line(char*line)
{
	url=strchr(line,' ');

	if(!url)
	{
		printf("5\n");
		return BAD_RESOURCE;
	}
	*url++='\0';
	method=line;
	if(strcasecmp(method,"GET")!=0)
	{
		printf("6\n");
		return BAD_RESOURCE;
	}
	while(*url==' ')url++;
	version=strchr(url,' ');
	*version++='\0';
	while(*version==' ')version++;
	if(strcasecmp(version,"HTTP/1.1")!=0)
	{
		return BAD_RESOURCE;
	}
	if(strcasecmp(url,"HTTP://")==0)//璇存槑url甯︽湁Http://
	{
		url+=7;
		url=strchr(url,'/');//鍦ㄨ繖绉嶆儏鍐典笅瑕侀櫎鍘诲煙鍚?	}
	if(!url||url[0]!='/')
	{
		printf("8\n");
		return BAD_RESOURCE;
	}
	url++;
	printf("%s\n",url);
	if(*url=='\0')
	{
		static char homepage[11]="./tmp.html";
		url=homepage;
	}
	char *p=NULL;
	if((p=strchr(url,'?'))!=NULL)
	{
		*p++='\0';
		cgiarg=p;
		printf("%s\n",url);
		printf("%s\n",cgiarg);
	}
	check_status=CHECK_HEAD;
	return UN_FINISH;
}
//鍙槸鍒ゆ柇鏈夋病鏈夊畬鏁磋鍏?Http_conn::HTTP_CODE Http_conn::request_content(char *line)
{
	if(contentline+read_index>read_size)//璇存槑娌℃湁瀹屾暣璇诲叆
	{
		return UN_FINISH;
	}
	line[contentline]='\0';
	return GET_RESOURCE;
}
Http_conn::HTTP_CODE Http_conn::request_head(char*line)
{
	if(line[0]=='\0')
	{
		if(contentline!=0)
		{
			check_status=CHECK_CONTENT;
			return UN_FINISH;
		}
		return GET_RESOURCE;
	}
	else if(strncasecmp(line,"Connection:",11)==0)
	{
		line+=11;
		while(*line==' ')line++;
		if(strncasecmp(line,"keep-alive",10)==0)
			keepalive=true;
	}
	else if(strncasecmp(line,"Content-length:",15)==0)
	{
		line+=15;
		while(*line==' ')line++;
		contentline=atoi(line);
	}
	else if(strncasecmp(line,"host:",5)==0)
	{
		line+=5;
		while(*line==' ')line++;
		m_host=line;
	}
	return UN_FINISH;
}
Http_conn::HTTP_CODE Http_conn::process_read()
{

	check_status=CHECK_LINE;
	HTTP_CODE ret;
	LINE_STATUS line_status;
	while((check_status==CHECK_CONTENT&&line_status==LINE_OK)||((line_status=parse_line())==LINE_OK))
	{
		char*text=m_start_line+read_buf;
		m_start_line=read_index;
		switch(check_status)
		{
			case CHECK_LINE:
				ret=request_line(text);
				if(ret==BAD_RESOURCE)
					return BAD_RESOURCE;
				break;
			case CHECK_HEAD:
				ret=request_head(text);
				if(ret==GET_RESOURCE)
					return do_request();//瑙ｆ瀽鍒ゆ柇鏂囦欢鏄惁鏈夌敤				
				break;
			case CHECK_CONTENT:
				ret=request_content(text);
				if(ret==GET_RESOURCE)
					return do_request();
				line_status=LINE_OPEN;
				break;
		}
	}
	if(line_status==LINE_OPEN)
	return UN_FINISH;//璇存槑杩樻病鏈夊畬鏁磋鍏?
	printf("4\n");
	return BAD_RESOURCE;
}
Http_conn::HTTP_CODE Http_conn::do_request()
{
	if(stat(url,&stat_buf)<0)
	{
		printf("%s\n",url);
		printf("1\n");
		return NO_RESOURCE;
	}
	if(!(stat_buf.st_mode&S_IROTH))//鍒ゆ柇鏄惁鏈夋潈闄?	{
		printf("2\n");
		return FORBIDDEN_RESOURCE;
	}
	if(!(S_ISREG(stat_buf.st_mode)))//鍒ゆ柇鏄惁鏄枃浠?	{
		printf("3\n");
		return BAD_RESOURCE;
	}
	howsend=1;
	if(cgiarg!=nullptr)
		return DYNAMIC_RESOURCE;
	return FILE_RESOURCE;
}
void Http_conn::process_write(HTTP_CODE status)
{
	switch (status)
	{
		case BAD_RESOURCE:
			clienterror("400","BAD REQUEST","your request has bad syntax");
			break;
		case DYNAMIC_RESOURCE:
			dynamic_request();
			break;
		case NO_RESOURCE:
			clienterror("404","NOT FOUND","couldn't found this file");
			break;
		case FORBIDDEN_RESOURCE:
			clienterror("403","FORBIDDEN","You couldn't read this file");
			break;
		case FILE_RESOURCE:
			success_file();
			break;
	}
}
void Http_conn::clienterror(const char *errnum, const char*shortmsg,const char*longmsg)
{
	sprintf(body_buf,"<html><title>ERROR></title>");
	sprintf(body_buf,"%s<body bgcolor=""ffffff"">\r\n",body_buf);
	sprintf(body_buf,"%s%s:	%s\r\n",body_buf,errnum,shortmsg);
	sprintf(body_buf,"%s<p>%s \r\n",body_buf,longmsg);
	sprintf(body_buf,"%s<hr><em>MY Web sever</em>\r\n",body_buf);


	sprintf(head_buf,"HTTP/1.1 %s %s\r\n",errnum,shortmsg);
	sprintf(head_buf,"%sContent-type:text/html\r\n",head_buf);
	sprintf(head_buf,"%sContent-length:%ld\r\n\r\n",head_buf,strlen(body_buf));
}
void Http_conn::success_file()
{
	sprintf(head_buf,"HTTP/1.0 200 Ok\r\n");
	sprintf(head_buf,"%sServer:LHB web server\r\n",head_buf);
	sprintf(head_buf,"%sConnection:close\r\n",head_buf);
	sprintf(head_buf,"%sContent-length:%ld\r\n\r\n",head_buf,stat_buf.st_size);
}

bool Http_conn::write()
{
	if(howsend==0)
	{
		if(rio_writen(head_buf,strlen(head_buf))==false)
			return false;
		return rio_writen(body_buf,strlen(body_buf));
	}
	else
	{
		if(rio_writen(head_buf,strlen(head_buf))==false)
			return false;
		int fd=open(url,O_RDONLY);
		char*buf=(char*)mmap(0,stat_buf.st_size,PROT_READ,MAP_PRIVATE,fd,0);
		::close(fd);
		bool issuccess=rio_writen(buf,stat_buf.st_size);
		munmap(buf,stat_buf.st_size);
		return issuccess;
	}
}
bool Http_conn::rio_writen(char* usrbuf,size_t n)
{
	char *buf=usrbuf;
	size_t left=n;
	size_t howsend;
	while(left>0)
	{
		if((howsend=::write(confd,buf,left))<0)
		{
			if(errno==EINTR)
				howsend=0;
			else
				return false;
		}
		left-=howsend;
		buf+=howsend;
	}
	return true;
}

void Http_conn::process()
{
	HTTP_CODE ret=process_read();
	if(ret==UN_FINISH)//璇存槑杩樻病鏈夊畬鏁磋鍏?	{
		modfd(epollfd,confd,EPOLLIN);
	}
	process_write(ret);
	if(ret==DYNAMIC_RESOURCE)
		return ;
	modfd(epollfd,confd,EPOLLOUT);
}
void Http_conn::init(int fd,struct sockaddr_in addr1)
{
	confd=fd;
	addr=addr1;
	usercout++;
	addfd(epollfd,fd,true);
	init();
}
void Http_conn::init()
{
	cgiarg=nullptr;
	read_index=0;
	read_size=0;
	method=nullptr;
	keepalive=false;
	url=nullptr;
	version=nullptr;
	m_start_line=0;
	contentline=0;
	m_host=nullptr;
	howsend=0;
	memset(read_buf,'\0',READSIZE);
	memset(head_buf,'\0',WRITESIZE);
	memset(body_buf,'\0',WRITESIZE);
}

void Http_conn::close()
{
	::close(confd);
	epoll_ctl(epollfd,confd,EPOLL_CTL_DEL,NULL);
	usercout--;
}
void Http_conn::dynamic_request()
{
	printf("dy\n");
	char buf[WRITESIZE];
	char *list[]={NULL};

	memset(buf,'\0',WRITESIZE);
	sprintf(buf,"HTTP/1.1 200 OK\r\n");
	sprintf(buf,"%sServer:my web server\r\n",buf);
	rio_writen(buf,strlen(buf));
	printf("%s\n",buf);
	pid_t pid;
	if((pid=fork())<0)
	{
		close();
	}
	else if(pid==0)
	{
		setenv("QUERY_STRING",cgiarg,1);
		dup2(confd,STDOUT_FILENO);
		execve(url,list,environ);
	}
	wait(NULL);
}







