# IPV4 over IPV6 隧道协议实验服务器端

## 1. 实验目的

​	IPV4 over IPV6，简称“4over6”是IPV4向IPV6发展进程中，向纯IPV6主干网过渡提出的一种新技术，可以最大程度地继承基于IPV4网络和应用，实现IPV4向IPV6平滑的过渡。

​	该实验通过实现IPV4 over IPV6隧道最小原型验证系统，让同学们对4over6隧道的实现原理有更加深刻的认识。

## 2.实验要求

在linux系统下，实现4over6隧道系统服务端程序，内容如下：

​	1）实现服务端与客户端之间控制通道的建立与维护；

​	2）实现对客户端网络接口的配置；

​	3）实现对4over6隧道系统数据报文的封装和解封装。

## 3.实验内容

服务端在linux环境下运行，主要有下面几个功能：

​	1）创建IPV6 TCP套接字，监听服务器和客户端之间的数据通信；

​	2）维护虚接口，实现对虚接口的读写操作；

​	3）维护IPV4地址池，实现为新连接客户端分配IPV4地址；

​	4）维护客户信息表，保存IPV4地址与IPV6套接字之间的映射关系；

​	5）读取客户端从IPV6 TCP套接字发送来的数据，实现对系统的控制消息和数据消息的处理；

​	6）实现对数据消息的解封装，并写入虚接口；

​	7）实现对虚接口接收到的数据报文进行封装，通过IPV6套接字发送给客户端；

​	8）实现保活机制，监测客户端是否在线，并且定时给客户端发送keeplive消息。

## 4.实验设计与实现

### 4.1结构体定义

服务端消息类型与客户端保持一致，结构体定义如下：

```
struct Msg
{
   int length;       //长度
   char type;    //类型
   char data[4096];   //数据段
   Msg(){
        length = 0;
        type = 0;
        memset(data,0,sizeof(data));
    }
};
```

服务端的消息类型如下表：

| 类型(char) | 长度(int) | 数据(char[4096]) | 备注        |
| -------- | ------- | -------------- | --------- |
| 100      |         | null           | 客户端IP地址请求 |
| 101      |         |                | IP地址回应    |
| 102      |         |                | 上网请求      |
| 103      |         |                | 上网回应      |
| 104      |         | null           | 心跳包       |

​	服务器收到客户端发送来的100类型的IP请求报文，会对其回应101类型报文，该报文的数据段，包含了IP地址、路由、三个DNS，服务器会以字符串的形式把这些信息写入报文数据段，格式为“IP 路由 DNS DNS DNS”，字符串之间以空格隔开，类似“13.8.0.2 0.0.0.0 202.38.120.242 8.8.8.8 202.106.0.20”。

​	每次连接过来一个客户端，服务器都会把该客户端的信息存储到一个客户信息表里，该信息表的定义如下：

```
struct User_Info_Table     //客户信息表
{
   int fd;                   //套接字描述符
   int count;                //标志位
   unsigned long int secs;          //上次收到keeplive时间
   struct in_addr v4addr;       //服务器给客户端分配的IPV4地址
   struct in6_addr v6addr;          //客户端的IPV6地址
   struct User_Info_Table * pNext;    //链表下一个节点
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
```

​	其中客户信息表中的count字段，是用来当一个计数器，当服务器给客户端发送完IP地址之后，会把该客户端的信息都存到客户信息表里，标志位赋值20（服务器每隔20秒给客户端发送心跳包），然后在keeplive线程中，每隔1秒，对所有客户端的count字段进行减1操作，当该客户端的count为0的时候，服务器才给该客户端发送心跳包消息。

​	服务端地址池定义：

```
struct IPADDR
{
   char addr[32];    //IP地址
   int status;          //标志位
};
```

