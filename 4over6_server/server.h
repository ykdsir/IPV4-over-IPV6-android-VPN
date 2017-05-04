#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <resolv.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include<sys/ioctl.h>
#include <fcntl.h>
#include <iostream>


struct Msg
{
	int length;		//长度
	char type;		//类型
	char data[4096];	//数据段
};
struct User_Info_Table 		//客户信息表
{
	int fd; 						//套接字描述符
	int count;						//标志位
	unsigned long int secs;			//上次收到keeplive时间
	struct in_addr v4addr;			//服务器给客户端分配的IPV4地址
	struct in6_addr v6addr;			//客户端的IPV6地址
	struct User_Info_Table * pNext;	//链表下一个节点
};
struct IPADDR
{
	char addr[32];		//IP地址
	int status;			//标志位
};
class User_Manager
{
public:
	User_Info_Table *head;//用户信息链表
	User_Info_Table *tail;

	User_Manager();
	void add_user(User_Info_Table user);
	void del_user(int sockfd);
	User_Info_Table *find_user(int sockfd);
};
