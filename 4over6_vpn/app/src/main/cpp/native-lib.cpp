#include <Android/log.h>
#include <jni.h>
#include <stdio.h>
#include <string>
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

#define IP_REQUEST 100
#define IP_RESPONCE 101
#define IT_REQUEST 102
#define IT_RESPONCE 103
#define HEARTBEAT 104

#define MAXBUFF 1024*128
#define DATA_SIZE 4096

std::string IP_TUNNEL = "/com.test.a4over6_vpn/ip_pipe";
std::string INFO_TUNNEL = "/com.test.a4over6_vpn/info_pipe";

struct Msg{
    int length;
    char type;
    char data[4096];
};


int sockfd;
time_t preHeartbeatTime = -1;
time_t curHeartbeatTime = -1;
bool socketLive = true;
void readTun(std::string tunnel,char* data,long length){

}

int writeTun(std::string tunnel,char* data,long length){
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

void setMsg(Msg* msg,char type,int data_length,char* data ){
    msg->type = type;
    if (data == NULL){
        data_length = 0;
    } else{
        memcpy(msg->data,0, sizeof(msg->data));
        memcpy(msg->data,data,(data_length));
    }
    msg->length = 5 + data_length;
}

void* heartbeatPackThr(void* args){
    printf("heart beat pack thread\n");
    return NULL;
}

void* dataPackThr(void* args){
    printf("data pack thread\n");
    //发送IP请求
    Msg ipRequest,reciveMsg;
    long length;
    setMsg(&ipRequest,IP_REQUEST,0,NULL);
    if(send(sockfd,&ipRequest, sizeof(Msg),0)<0){
        return strerror(errno);
    }

    char recvBuff[MAXBUFF];
    preHeartbeatTime = time(NULL);
    while (1){
        if(!socketLive)
            break;
        length = recv(sockfd,recvBuff,0, sizeof(recvBuff));
        memcpy(&reciveMsg,recvBuff, sizeof(reciveMsg));
        switch (reciveMsg.type){
            case IP_RESPONCE:
                char ip[20],router[20],dns1[20],dns2[20],dns3[20];
                sscanf(reciveMsg.data,"%s %s %s %s %s",ip,router,dns1,dns2,dns3);
                char toWrite[DATA_SIZE];
                sprintf(toWrite, "%s %s %s %s %s %d", ip, router, dns1, dns2, dns3, sockfd);
                writeTun(IP_TUNNEL,toWrite,strlen(toWrite));
                //TODO 读取前台传递的虚接口，封装102类型报文
                break;
            case IT_RESPONCE:
                break;
            case HEARTBEAT:
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

extern "C"
JNIEXPORT jstring JNICALL
Java_com_test_a4over6_1vpn_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_test_a4over6_1vpn_MainActivity_startBackground(JNIEnv *env, jobject instance) {
    // Done 创建ipv6套接字，连接隧道服务器
    //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
    std::string returnValue = "ok\n";
    struct sockaddr_in6 server;
    bzero(&server, sizeof(server)); //把一段内存区的内容全部设置为0
    server.sin6_family = AF_INET6; //internet协议族
    server.sin6_port = htons(5678); //0表示让系统自动分配一个空闲端口
    if (inet_pton(AF_INET6, "2402:f000:1:4417::900", &server.sin6_addr) < 0)
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
        return env->NewStringUTF(returnValue.c_str());
    }

    //管道是否创建
    if (access(IP_TUNNEL.c_str(), F_OK) == -1)
    {
        mknod(IP_TUNNEL.c_str(), S_IFIFO | 0666, 0);
    }
    if (access(INFO_TUNNEL.c_str(), F_OK) == -1)
    {
        mknod(INFO_TUNNEL.c_str(), S_IFIFO | 0666, 0);
    }

    //创建两个线程，读取数据和发送心跳包
    pthread_t data,heartbeat;
    int data_err,heart_err;
    data_err = pthread_create(&data,NULL,dataPackThr,NULL);
    heart_err = pthread_create(&heartbeat,NULL,heartbeatPackThr,NULL);
    if(data_err!=0 || heart_err!=0){
        printf("create thraed error\n");
    }
    pthread_join(data, NULL);
    pthread_join(heartbeat, NULL);
    return env->NewStringUTF(returnValue.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_test_a4over6_1vpn_MainActivity_test(JNIEnv *env, jobject instance) {


    std::string returnValue = "test";
    return env->NewStringUTF(returnValue.c_str());
}