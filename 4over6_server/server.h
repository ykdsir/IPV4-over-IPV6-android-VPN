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
	User_Info_Table(){
		this->pNext = NULL;
	}
	User_Info_Table(const User_Info_Table& user){
		this->fd = user.fd;
		this->count = user.count;
		this->secs = user.secs;
		this->v4addr = user.v4addr;
		this->v6addr = user.v6addr;
		this->pNext = user.pNext;
	}
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
//typedef struct tIPPackHead
//{
//
//    char ver_hlen;      //IP协议版本和IP首部长度。高4位为版本，低4位为首部的长度(单位为4 bytes)
//    char byTOS;       //服务类型
//    char wPacketLen[2]; //IP包总长度。包括首部，单位为byte。[Big endian]
//    char wSequence[2];    //标识，一般每个IP包的序号递增。[Big endian]
//
//    union
//    {
//    	char Flags[2]; //标志
//    	char FragOf[2];//分段偏移
//    };
//    char byTTL;         //生存时间
//    char byProtocolType; //协议类型，见PROTOCOL_TYPE定义
//    char wHeadCheckSum[2];    //IP首部校验和[Big endian]
//    char dwIPSrc[4];         //源地址
//    char dwIPDes[4];         //目的地址
//    char Options;          //选项
//} IP_HEAD;
struct IP_HEAD
{
    char ver_hlen;     //版本信息(前4位)，头长度(后4位)
    char byTOS;       // 服务类型8位
    short wPacketLen;    //数据包长度
    short wSequence;     //数据包标识
    short m_sSliceinfo;    //分片使用
    char byTTL;        //存活时间
    char byProtocolType;    //协议类型
    short wHeadCheckSum;       //校验和
    unsigned int dwIPSrc;    //源ip
    unsigned int dwIPDes;    //目的ip
};


struct TCP_HEAD
{
    short m_sSourPort;        // 源端口号16bit
    short m_sDestPort;        // 目的端口号16bit
    unsigned int m_uiSequNum;       // 序列号32bit
    unsigned int m_uiAcknowledgeNum;  // 确认号32bit
    short m_sHeaderLenAndFlag;     // 前4位：TCP头长度；中6位：保留；后6位：标志位
    short m_sWindowSize;      // 窗口大小16bit
    short m_sCheckSum;         // 检验和16bit
    short m_surgentPointer;      // 紧急数据偏移量16bit
};