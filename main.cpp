#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include"http.h"
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include"pthreadpool.h"
#define port 8888
const int MAXEVENT=10000;
const int MAXCONNECT=60000;
extern void addfd(int epollfd,int fd,bool oneshot);
int main(int argc,char*argv[])
{
	struct sockaddr_in ser_addr;
	int cout1=0;
	bzero(&ser_addr,sizeof(ser_addr));
	ser_addr.sin_family=AF_INET;
	ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	ser_addr.sin_port=htons(port);
	int serverfd=socket(AF_INET,SOCK_STREAM,0);
	if(serverfd<0)
	{
		printf("socket failed:%s\n",strerror(errno));
		exit(0);
	}
	if(bind(serverfd,(sockaddr*)&ser_addr,sizeof(ser_addr))<0)
	{
		printf("bind error:%sn",strerror(errno));
		exit(0);
	}
	if(listen(serverfd,1024)<0)
	{
		printf("listen error:%sn",strerror(errno));
		exit(0);
	}
	int epollfd=epoll_create(MAXCONNECT);
	addfd(epollfd,serverfd,false);
	Http_conn::epollfd=epollfd;
	struct sockaddr_in client_addr;	
	socklen_t clientlen=sizeof(client_addr);
	Http_conn* users=new Http_conn[MAXCONNECT];
	if(users==nullptr)
	{
		printf("new error\n");
		exit(0);
	}
	Pthreadpool<Http_conn>* pl=nullptr;
	pl=new Pthreadpool<Http_conn>(8,MAXEVENT);
	if(pl==nullptr)
	{
		printf("new error\n");
		exit(0);
	}
	struct epoll_event events[MAXEVENT];

	while(1)
	{
		printf("coneto to\n");
		int count=epoll_wait(epollfd,events,MAXEVENT ,-1);
		for(int i=0;i<count;i++)
		{
			int sockfd=events[i].data.fd;
			if(sockfd==serverfd)
			{
					
				printf("cout:%d\n",cout1++);
				printf("cout1%d\n",Http_conn::usercout);
				int confd=accept(sockfd,(sockaddr*)&client_addr,&clientlen);
				if(confd<0)
				{
					printf("accept error\n");
					continue;
				}
				//濡傛灉杩炴帴鏁伴噺澶т簬
				if(Http_conn::usercout>=MAXCONNECT)
				{
					write(confd,"serverbusy",10);
					close(confd);
					continue;
				}
				users[confd].init(confd,client_addr);
			}
			else if(events[i].events&EPOLLHUP)
			{
				printf("HUP\n");
				users[sockfd].close();
			}
			else if(events[i].events&EPOLLIN)
			{
				printf("IN\n");
				if(users[sockfd].read()==true)//濡傛灉璇诲彇鏁版嵁鎴愬姛鍔犺繘绾跨▼姹?				{
					pl->push(users+sockfd);
				}
				else
				{
					users[sockfd].close();
				}
			}
			else if (events[i].events&EPOLLOUT)
			{
				printf("OUT\n");
				users[sockfd].write();
				users[sockfd].close();
			}
		}
	}
	
	close(epollfd);
	close(serverfd);
	delete users;
	delete pl;
	return 0;



	




}
