#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <netdb.h>
#include <iostream>
#define IP_REQUEST 100
#define IP_RESPONCE 101
#define IT_REQUEST 102
#define IT_RESPONCE 103
#define HEARTBEAT 104

#define MAXBUFF 1024*128
#define DATA_SIZE 4096
// #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"background",__VA_ARGS__)

struct Msg{
    int length;
    char type;
    char data[DATA_SIZE];
    Msg(){
        memset(data,0,DATA_SIZE);
    }
};

int sockfd;//套接字接口
time_t preHeartbeatTime = -1;//上一次心跳时间
time_t curHeartbeatTime = -1;//当前心跳时间
bool socketLive = true;//socket是否存活
int virIntFileDescriptor = -1;//虚接口描述符
long recvLength = 0;
long recvTotalLength = 0;
long recvCount = 0;
long sendLength = 0;
long sendTotalLength = 0;
long sendCount = 0;


//设置Msg结构体的信息
void setMsg(Msg* msg,char type,int data_length,char* data ){
    msg->type = type;
    if (data == NULL){
        data_length = 0;
    } else{
        memset(msg->data,0, sizeof(msg->data));
        memcpy(msg->data,data,(data_length));
    }
    msg->length = 5 + data_length;
}

//读管道
void readTun(std::string tunnel,char* data,unsigned long length){
    int fifo_handle = open(tunnel.c_str(),O_RDONLY);
    if(fifo_handle != -1){
        if((read(fifo_handle,data,length) )< 0){
            printf("read tunnel error\n");
        }
    } else{
        printf("open tunnel error\n");
    }
}

//写管道
int writeTun(std::string tunnel,char* data,unsigned long length){
    int fifo_handle = open(tunnel.c_str(),O_RDWR|O_CREAT|O_TRUNC);
    long writein = 0,extra = 0;
    if (fifo_handle > 0){
        while (extra < length){
            writein = write(fifo_handle,data,length);
            if(writein < 0)
                break;
            extra += writein;
        }
        close(fifo_handle);
        return 0;
    } else{
        return -1;
    }
}

//读虚接口
void* readVirtualInterfaceThr(void* args){
    char readFromVI[DATA_SIZE];
    while (1){
        if(!socketLive)
            break;
        memset(readFromVI,0,DATA_SIZE);
        long length = read(virIntFileDescriptor,readFromVI,DATA_SIZE);
        if(length > 0){
            Msg sendPkg;
            setMsg(&sendPkg,IT_REQUEST,strlen(readFromVI),readFromVI);
            if(send(sockfd,&sendPkg, sizeof(Msg),0)){
//                return strerror(errno);
                printf("%s\n",strerror(errno));
                continue;
            }
            sendLength+=length;
            sendTotalLength += length;
            ++sendCount;
        }
    }
    return NULL;
}

void* heartbeatPackThr(void* args){
    printf("heart beat pack thread\n");
    preHeartbeatTime = time(NULL);
    int counter = 0;
    Msg counterHeartbeat;
    time_t curTime;
    char toFront[MAXBUFF];//传递给前台的相关数据
    long uploadSpeed = 0,downloadSpeed = 0;
    time_t interval;
    while (1){
        if (!socketLive){
            printf("heart beat package socketlive failed!\n");
            return NULL;
        }
        sleep(1);
        curTime = time(NULL);//当前时间
        interval = curTime - preHeartbeatTime;
        if(interval > 60){
            socketLive = false;
            return NULL;
        } else{
            uploadSpeed = downloadSpeed = 0;
            //TODO 流量统计传送到前台
            /*
             * 上传、下载速度
             * 上传总流量和包数
             * 下载总流量和包数
             * IPV4，IPV6地址？？
             */
            uploadSpeed =(sendLength/1024)/interval;
            downloadSpeed = (recvLength/1024)/interval;
            sprintf(toFront,"%ld %ld %ld %ld %ld %ld",uploadSpeed,downloadSpeed,sendTotalLength,sendCount,recvTotalLength,recvCount);
            // writeTun(INFO_TUNNEL,toFront,strlen(toFront));//将信息写入流量信息管道
            sendLength = 0;
            recvLength = 0;
            ++counter;
            if (counter >= 20){
                setMsg(&counterHeartbeat,HEARTBEAT,0,NULL);
                if(send(sockfd,&counterHeartbeat, sizeof(Msg),0) < 0 ) {
                    printf("%s\n",strerror(errno));
                    return NULL;
                }
                counter = 0;
            }
        }
    }

    return NULL;
}

void* dataPackThr(void* args){

    Msg ipRequest,reciveMsg;
    long length;
    pthread_t virIntThread;
    char recvBuff[MAXBUFF];
    char toWrite[DATA_SIZE];

    //发送IP请求
    setMsg(&ipRequest,IP_REQUEST,0,NULL);
    if(send(sockfd,&ipRequest, sizeof(Msg),0)<0){
        printf("%s\n", strerror(errno));
    }
    printf("data pack thread\n");
    while (1){
        if(!socketLive)
            break;
        //接收服务器回复
        memset(recvBuff,0,MAXBUFF* sizeof(char));
        memset(toWrite,0,DATA_SIZE);
//        printf("before recv");
        length = recv(sockfd,recvBuff, sizeof(recvBuff),MSG_DONTWAIT|MSG_PEEK);
        if(length<0)
            continue;
//        printf("after recv %d",length);
        memcpy(&reciveMsg,recvBuff, sizeof(reciveMsg));
        switch (reciveMsg.type){
            case IP_RESPONCE:
                // printf("ip responce");
                char ip[20],router[20],dns1[20],dns2[20],dns3[20];
                sscanf(reciveMsg.data,"%s %s %s %s %s",ip,router,dns1,dns2,dns3);

                sprintf(toWrite, "%s %s %s %s %s %d", ip, router, dns1, dns2, dns3, sockfd);
                printf("recieve: %s",reciveMsg.data);
                // writeTun(IP_TUNNEL,toWrite,strlen(toWrite));
                //Done 读取前台传递的虚接口，封装102类型报文
                // memset(recvBuff,0,MAXBUFF* sizeof(char));//此时recvBuff暂时用来保存读取虚接口的文件描述符
                // readTun(IP_TUNNEL,recvBuff,MAXBUFF);
                // sscanf(recvBuff,"%d",&virIntFileDescriptor);

                // pthread_create(&virIntThread,NULL,readVirtualInterfaceThr,NULL);
                // pthread_join(virIntThread,NULL);
                break;

            case IT_RESPONCE:
                // printf("IT_RESPONCE");
                sscanf(reciveMsg.data,"%s",toWrite);
                if(write(virIntFileDescriptor,toWrite,DATA_SIZE) < 0){
                    return strerror(errno);
                } else{
                    ++recvCount;
                    recvLength += strlen(toWrite);
                    recvTotalLength += strlen(toWrite);
                }
                break;

            case HEARTBEAT:
                // printf("heartbeat");
                curHeartbeatTime = time(NULL);
                if (curHeartbeatTime - preHeartbeatTime > 60){
                    socketLive = false;
                }
                preHeartbeatTime = curHeartbeatTime;
                break;
            default:
                break;
        }
    }
    return NULL;
}

int main(){
    // Done 创建ipv6套接字，连接隧道服务器
    //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
    std::string returnValue = "ok\n";
    struct sockaddr_in6 server;
    bzero(&server, sizeof(server)); //把一段内存区的内容全部设置为0

   server.sin6_family = AF_INET6; //internet协议族
    server.sin6_port = htons(9734); //0表示让系统自动分配一个空闲端口
    if (inet_pton(AF_INET6, "0:0:0:0:0:0:0:1", &server.sin6_addr) < 0)
    {
        returnValue = ("Net address error\n");
    }
    if ((sockfd = socket(AF_INET6,SOCK_STREAM,0))<0) {
        returnValue += ("create socket error\n");
    }
    //连接服务器
    int temp = connect(sockfd, (struct sockaddr *) &server, sizeof(server));
    if(temp < 0) {
        returnValue += ("connet to server error\n");
        return -1;
    }
     printf("after connect \n");
    //管道是否创建
  //   string msg = "hello,world";
  //   std::string msg = "abcdeg";
   //  write(sockfd,msg.c_str(),sizeof(msg.c_str()));

    //Done 创建两个线程，读取数据和发送心跳包
    pthread_t data,heartbeat;
    int data_err,heart_err;
    data_err = pthread_create(&data,NULL,dataPackThr,NULL);
    heart_err = 0;//pthread_create(&heartbeat,NULL,heartbeatPackThr,NULL);
    if(data_err!=0 || heart_err!=0){
        printf("create thraed error\n");
    }
    pthread_join(data, NULL);
    //pthread_join(heartbeat, NULL);

    return 0;
}
