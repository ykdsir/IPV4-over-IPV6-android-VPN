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
#define DATA_SIZE 40960
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"background",__VA_ARGS__)

std::string IP_TUNNEL = "/data/data/com.test.a4over6_vpn/ip_pipe";
std::string INFO_TUNNEL = "/data/data/com.test.a4over6_vpn/info_pipe";

struct Msg{
    int length;
    char type;
    char data[DATA_SIZE];
};

int sockfd;//套接字接口
time_t preHeartbeatTime = -1;//上一次心跳时间
time_t curHeartbeatTime = -1;//当前心跳时间
bool socketLive = false;//socket是否存活
int virIntFileDescriptor = -1;//虚接口描述符
double recvLength = 0;
double recvTotalLength = 0;
double recvCount = 0;
double sendLength = 0;
double sendTotalLength = 0;
double sendCount = 0;
bool  IPTunnelChanged = false;
bool getFD = false;
//设置Msg结构体的信息
void setMsg(Msg* msg,char type,int data_length,char* data ){
    msg->type = type;
    if (data == NULL){
        data_length = 0;
    } else{
        memset(msg->data,0, sizeof(msg->data));
        memcpy(msg->data,data,(unsigned long)(data_length));
    }
    msg->length = data_length+5;
}

//读管道
void readTun(std::string tunnel,char* data,unsigned long length){
    int fifo_handle = open(tunnel.c_str(),O_RDONLY);
    if(fifo_handle != -1){
        if((read(fifo_handle,data,length) )< 0){
            LOGD("read tunnel error\n");
        }
    } else{
        LOGD("open tunnel error\n");
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
    long length;
    Msg sendPkg;
    LOGD("virtual descriptor: %d\n",virIntFileDescriptor);
    while (1){
        if(!socketLive) {
            break;
        }
        memset(readFromVI,0,DATA_SIZE);
        length = read(virIntFileDescriptor,readFromVI, sizeof(readFromVI));
        if(length > 0){
//            LOGD("read from IV %s length: %ld\n",readFromVI,length);
//            for(int k=0;k<length;++k){
//                LOGD("%02x",readFromVI+k);
//            }
//            setMsg(&sendPkg,IT_REQUEST,strlen(readFromVI),readFromVI);
            setMsg(&sendPkg,IT_REQUEST,length,readFromVI);
//            LOGD("length: %d\n",sendPkg.length);
            if((send(sockfd,&sendPkg, sizeof(Msg),0))<0){
                LOGD("send 102 %s\n",strerror(errno));
                continue;
            };
            sendLength+=length;
            sendTotalLength += length;
            ++sendCount;
//            return NULL;
        }
    }
    return NULL;
}

void* heartbeatPackThr(void* args){
    LOGD("heart beat pack thread\n");
    preHeartbeatTime = time(NULL);
    int counter = 0;
    Msg counterHeartbeat;
    time_t curTime;
    char toFront[MAXBUFF];//传递给前台的相关数据
    double uploadSpeed = 0,downloadSpeed = 0;
    time_t interval;
    while (1){
        if (!socketLive){
            LOGD("heart beat package socketlive failed!\n");
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
             */
            uploadSpeed =(sendLength/1024)/interval;
            downloadSpeed = (recvLength/1024)/interval;
            sprintf(toFront,"%lf %lf %lf %lf %lf %lf",uploadSpeed,downloadSpeed,sendTotalLength,sendCount,recvTotalLength,recvCount);
//            LOGD("%lf %lf %lf %lf %lf %lf",uploadSpeed,downloadSpeed,sendTotalLength,sendCount,recvTotalLength,recvCount);
            writeTun(INFO_TUNNEL,toFront,strlen(toFront));//将信息写入流量信息管道
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
    LOGD("data pack thread\n");

    Msg ipRequest,reciveMsg;
    long length;
    long readLength;
    pthread_t virIntThread;
    char recvBuff[MAXBUFF];
    char toWrite[DATA_SIZE];

    //发送IP请求
    setMsg(&ipRequest,IP_REQUEST,0,NULL);
    if((length = send(sockfd,&ipRequest, sizeof(Msg),0))<0){
        LOGD("%s\n", strerror(errno));
    }

    while (1){
        if(!socketLive)
            break;
        //接收服务器回复
        memset(recvBuff,0,MAXBUFF* sizeof(char));
        memset(&reciveMsg,0, sizeof(Msg));
        memset(toWrite,0,DATA_SIZE);
//        LOGD("before recv");
        length = recv(sockfd,recvBuff, sizeof(recvBuff),0);
        if(length<0)
            if ((errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
                continue;
            else
                return NULL;
        LOGD("after recv %ld",length);
//        for(int i=0;i<length;++i){
//            LOGD("%02x ",recvBuff[i]);
//        }
//        continue;
        if(((Msg*)(recvBuff))->length!=0)
            readLength = ((Msg*)(recvBuff))->length;
        memcpy(&reciveMsg,recvBuff, readLength);
//        LOGD("type:%02x",(reciveMsg.type));
//        LOGD("length:%d",reciveMsg.length);
        LOGD("read length:%d",readLength);
//        LOGD("message:%s",reciveMsg.data);
//        LOGD("type:%02x",((Msg*)(recvBuff))->type);
//        LOGD("length:%d",((Msg*)(recvBuff))->length);
//        LOGD("message:%s",((Msg*)(recvBuff))->data);
//        LOGD("raw message:%s",recvBuff);
        switch (reciveMsg.type){
            case IP_RESPONCE:
                LOGD("ip responce\n");
                char ip[20],router[20],dns1[20],dns2[20],dns3[20];
                sscanf(reciveMsg.data,"%s %s %s %s %s",ip,router,dns1,dns2,dns3);
                sprintf(toWrite, "%s %s %s %s %s %d", ip, router, dns1, dns2, dns3, sockfd);
                LOGD("recieve: %s\n",reciveMsg.data);
                writeTun(IP_TUNNEL,toWrite,strlen(toWrite));
                IPTunnelChanged = true;
                //Done 读取前台传递的虚接口，封装102类型报文
                memset(recvBuff,0,MAXBUFF* sizeof(char));//此时recvBuff暂时用来保存读取虚接口的文件描述符
//                LOGD("before read: %s\n",recvBuff);
                while (!getFD){
                    sleep(1);
                }
                readTun(IP_TUNNEL,recvBuff,MAXBUFF);
                LOGD("after read: %s\n",recvBuff);
                sscanf(recvBuff,"%d",&virIntFileDescriptor);
                LOGD("virIntFileDescriptor: %d\n",virIntFileDescriptor);
                memset(recvBuff,0,MAXBUFF* sizeof(char));
                writeTun(IP_TUNNEL,recvBuff,strlen(recvBuff));
                pthread_create(&virIntThread,NULL,readVirtualInterfaceThr,NULL);
                break;

            case IT_RESPONCE:
                LOGD("IT_RESPONCE");
//                sscanf(recvBuff,"%s",toWrite);
                LOGD("recv length:%d\n",reciveMsg.length);
                LOGD("part data:%s\n",reciveMsg.data);
                LOGD("%d\n",virIntFileDescriptor);
                memcpy(toWrite,reciveMsg.data,reciveMsg.length);
//                __android_log_assert(virIntThread == -1,"ykd",NULL);
                if(write(virIntFileDescriptor,&toWrite[0], ((size_t)reciveMsg.length)) < 0){
                    LOGD("write virtunnel wrong %s\n",strerror(errno));
                    return strerror(errno);
                } else{
                    ++recvCount;
                    recvLength += length;
                    recvTotalLength += length;
                }
                break;

            case HEARTBEAT:
                LOGD("heartbeat");
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
JNIEXPORT void JNICALL
Java_com_test_a4over6_1vpn_MainActivity_disConnect(JNIEnv *env, jobject instance) {

    // TODO Disconnect
    IPTunnelChanged = false;
    getFD = false;
    preHeartbeatTime = -1;//上一次心跳时间
    curHeartbeatTime = -1;//当前心跳时间
    socketLive = false;//socket是否存活
    virIntFileDescriptor = -1;//虚接口描述符
    recvLength = 0;
    recvTotalLength = 0;
    recvCount = 0;
    sendLength = 0;
    sendTotalLength = 0;
    sendCount = 0;
    close(sockfd);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_test_a4over6_1vpn_MyVPNService_setFD(JNIEnv *env, jobject instance) {
    getFD = true;
    return ;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_test_a4over6_1vpn_MainActivity_IsIPTunnelChanged(JNIEnv *env, jobject instance) {
    jboolean tRet = JNI_FALSE;
    if(IPTunnelChanged)
        tRet = JNI_TRUE;
    return tRet;
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_test_a4over6_1vpn_MainActivity_startBackground(JNIEnv *env, jobject instance) {
    // Done 创建ipv6套接字，连接隧道服务器
    //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
    std::string returnValue = "ok\n";
    socketLive = true;
    struct sockaddr_in6 server;
    bzero(&server, sizeof(server)); //把一段内存区的内容全部设置为0
    server.sin6_family = AF_INET6; //internet协议族
    server.sin6_port = htons(45000); //0表示让系统自动分配一个空闲端口
//    server.sin6_port = htons(9734); //0表示让系统自动分配一个空闲端口

    if (inet_pton(AF_INET6, "2402:f000:5:8601:3a9c:e094:615a:f970", &server.sin6_addr) < 0)
//    if (inet_pton(AF_INET6, "2402:f000:1:4417::900", &server.sin6_addr) < 0)
    {
        returnValue = ("Net address error\n");
    }
    if ((sockfd = socket(AF_INET6,SOCK_STREAM,0))<0) {
        returnValue += ("create socket error\n");
    }
    //连接服务器
    int temp = connect(sockfd, (struct sockaddr *) &server, sizeof(server));
    if(temp < 0) {
        LOGD("%s",strerror(errno));
        returnValue += ("connet to server error\n");
        return env->NewStringUTF(returnValue.c_str());
    }
    LOGD("after connect \n");
    //管道是否创建
    if (access(IP_TUNNEL.c_str(), F_OK) == -1)
    {
        mknod(IP_TUNNEL.c_str(), S_IFIFO | 0666, 0);
    }
    if (access(INFO_TUNNEL.c_str(), F_OK) == -1)
    {
        mknod(INFO_TUNNEL.c_str(), S_IFIFO | 0666, 0);
    }

    //Done 创建两个线程，读取数据和发送心跳包
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