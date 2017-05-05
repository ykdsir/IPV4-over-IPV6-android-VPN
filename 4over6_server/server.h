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

//IP首部
typedef struct tIPPackHead
{

    char ver_hlen;      //IP协议版本和IP首部长度。高4位为版本，低4位为首部的长度(单位为4 bytes)
    char byTOS;       //服务类型
    char wPacketLen[2]; //IP包总长度。包括首部，单位为byte。[Big endian]
    char wSequence[2];    //标识，一般每个IP包的序号递增。[Big endian]

    union
    {
    	char Flags[2]; //标志
    	char FragOf[2];//分段偏移
    };
    char byTTL;         //生存时间
    char byProtocolType; //协议类型，见PROTOCOL_TYPE定义
    char wHeadCheckSum[2];    //IP首部校验和[Big endian]
    char dwIPSrc[4];         //源地址
    char dwIPDes[4];         //目的地址
    char Options;          //选项
} IP_HEAD;
