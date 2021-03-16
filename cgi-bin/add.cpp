#include<cstring>
#include<stdlib.h>
#include<stdio.h>
int main()
{
	int m=0, n=0;
	char *buf;
	char content[1024];
	char *arg1,*arg2;
	if((buf=getenv("QUERY_STRING"))!=NULL)
	{
		char*p=strchr(buf,'&');
		*p++='\0';
		arg1=strchr(buf,'=');
		arg1++;
		arg2=strchr(p,'=');
		arg2++;
		m=atoi(arg1);
		n=atoi(arg2);
	}
	sprintf(content,"wecome to add.com:<p></p>");
	sprintf(content,"%sthe anwser is:%d+%d=%d\r\n<p></p>",content,m,n,m+n);
	sprintf(content,"%sthank for your visiting!\r\n",content);

	printf("Connection:close\r\n");
	printf("Content-length:%d\r\n",(int)strlen(content));
	printf("Content-type:text/html\r\n\r\n");
	printf("%s",content);
	fflush(stdout);
			
	return 0;
}
