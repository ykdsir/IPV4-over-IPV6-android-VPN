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

fd_set readfds;
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


void disConnect(){
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
            setMsg(&sendPkg,IT_REQUEST,length,readFromVI);
//            LOGD("length: %d\n",sendPkg.length);
            if((send(sockfd,&sendPkg, sendPkg.length,0))<0){
                LOGD("send 102 %s\n",strerror(errno));
                continue;
            };
            sendLength+=length;
            sendTotalLength += length;
            ++sendCount;
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
    double sendtotal = 0,recvtotal = 0;
    char sendunit = 'K',recvunit = 'K';
    time_t interval;
    while (1){
        if (!socketLive){
            LOGD("heart beat package socketlive failed!\n");
            break;
        }
        sleep(1);
        curTime = time(NULL);//当前时间
        interval = curTime - preHeartbeatTime;
        if(interval > 60){
            socketLive = false;
            disConnect();
            return NULL;
        } else{
            //TODO 流量统计传送到前台
            /*
             * 上传、下载速度
             * 上传总流量和包数
             * 下载总流量和包数
             */
            uploadSpeed =(sendLength);
            downloadSpeed = (recvLength/1024);
            sendtotal = sendTotalLength/1024;
            recvtotal = recvTotalLength/1024;
            if(sendtotal > (1024*1024)){
                sendtotal = sendtotal/1024;
                sendunit = 'M';
            }
            if(recvTotalLength > (1024*1024)){
                recvtotal = recvtotal/1024;
                recvunit = 'M';
            }
            sprintf(toFront,"%lf %lf %lf %lf %lf %lf %c %c",uploadSpeed,downloadSpeed,sendtotal,sendCount,recvtotal,recvCount,sendunit,recvunit);
            LOGD(toFront,"%lf %lf %lf %lf %lf %lf %c %c",uploadSpeed,downloadSpeed,sendtotal,sendCount,recvtotal,recvCount,sendunit,recvunit);
            writeTun(INFO_TUNNEL,toFront,strlen(toFront));//将信息写入流量信息管道
            sendLength = 0;
            recvLength = 0;
            ++counter;
            if (counter >= 20){
                setMsg(&counterHeartbeat,HEARTBEAT,0,NULL);
                if(send(sockfd,&counterHeartbeat, counterHeartbeat.length,0) < 0 ) {
                    printf("%s\n",strerror(errno));
                    return NULL;
                }
                counter = 0;
            }
        }
    }

    return 0;
}


void handleMsg(Msg reciveMsg){
    ++recvCount;
    recvLength += reciveMsg.length;
    recvTotalLength += reciveMsg.length;
    char toWrite[MAXBUFF];
    memset(toWrite,0,MAXBUFF);
    switch (reciveMsg.type){
        case IP_RESPONCE:
            LOGD("ip responce\n");
            pthread_t virIntThread;
            char ip[20],router[20],dns1[20],dns2[20],dns3[20];
            sscanf(reciveMsg.data,"%s %s %s %s %s",ip,router,dns1,dns2,dns3);
            sprintf(toWrite, "%s %s %s %s %s %d", ip, router, dns1, dns2, dns3, sockfd);
            LOGD("recieve: %s\n",reciveMsg.data);
            writeTun(IP_TUNNEL,toWrite,strlen(toWrite));
            IPTunnelChanged = true;
            //Done 读取前台传递的虚接口，封装102类型报文
            memset(toWrite,0,MAXBUFF);
            LOGD("before read: %s\n",toWrite);
            while (!getFD){
                sleep(1);
            }
            readTun(IP_TUNNEL,toWrite,MAXBUFF);
            LOGD("after read: %s\n",toWrite);
            sscanf(toWrite,"%d",&virIntFileDescriptor);
            LOGD("virIntFileDescriptor: %d\n",virIntFileDescriptor);
            memset(toWrite,0,MAXBUFF* sizeof(char));
            writeTun(IP_TUNNEL,toWrite,strlen(toWrite));
            pthread_create(&virIntThread,NULL,readVirtualInterfaceThr,NULL);
            break;

        case IT_RESPONCE:
//            LOGD("IT_RESPONCE");
//                sscanf(recvBuff,"%s",toWrite);
//            LOGD("recv length:%d\n",reciveMsg.length);
//            LOGD("part data:%s\n",reciveMsg.data);
//            LOGD("%d\n",virIntFileDescriptor);
            memcpy(toWrite,reciveMsg.data,reciveMsg.length);
            if(virIntFileDescriptor < 0)
                return;
//                __android_log_assert(virIntThread == -1,"ykd",NULL);
            if(write(virIntFileDescriptor,&toWrite[0], ((size_t)reciveMsg.length)) < 0){
                LOGD("write virtunnel wrong %s\n",strerror(errno));
            }
            break;

        case HEARTBEAT:
//            LOGD("heartbeat");
            curHeartbeatTime = time(NULL);
            if (curHeartbeatTime - preHeartbeatTime > 60){
                socketLive = false;
            }
            preHeartbeatTime = curHeartbeatTime;
            break;
        default:
            break;
    }
    return;
}

void* dataPackThr(void* args){
    LOGD("data pack thread\n");
    Msg ipRequest,reciveMsg;
    long length;
    long readLength;

    char recvBuff[MAXBUFF];


    //发送IP请求
    setMsg(&ipRequest,IP_REQUEST,0,NULL);
    if((length = send(sockfd,&ipRequest, ipRequest.length,0))<0){
        LOGD("%s\n", strerror(errno));
    }
    int res = 0;//剩余的包的长度
    int resLength = 0;//结束时未收完的下一个包的长度
    char pre[DATA_SIZE];
    memset(pre,0,DATA_SIZE* sizeof(char));
    while (1){
        if(!socketLive) {
            break;
        }
        //接收服务器回复
        memset(recvBuff,0,MAXBUFF* sizeof(char));
        memset(&reciveMsg,0, sizeof(Msg));
        //--------------
        int num = 0;
        try {
            while(num<4){
                num = recv(sockfd,&reciveMsg,4,MSG_PEEK);
                if(num==0){
                    LOGD("recv_Msg from %d error : %s,\n", sockfd, strerror(errno));
                }
                if(num<0){
                    if(errno != EINTR && errno != 0){
                        LOGD("recv_Msg from %d failed errno : %s, \n" , sockfd,strerror(errno));
                        FD_CLR(sockfd,&readfds);
                        try{
                            close(sockfd);
                            disConnect();
                            return NULL;
                        }catch (...){
                            LOGD("main close fd failed \n");
                        }
                    }
                }
            }
            int length = reciveMsg.length;
            num = 0;
            while(num < length){
                int temp = recv(sockfd,&reciveMsg+num,length-num,MSG_WAITALL);
                if(temp > 0){
                    num += temp;
                }else{
                    printf("temp < 0: num:%d / length:%d error : %s\n", num,length, strerror(errno));
                    break;
                }
                //num += recv(fd,reciveMsg+num,length-num,MSG_WAITALL);
            }
            handleMsg(reciveMsg);
            //num = recv(fd,msg,length,MSG_WAITALL);
        }catch (...) {
            printf("recv_Msg from %d error : %s", sockfd, strerror(errno));
            return NULL;
        }

        //---------------

    }
    return 0;
}



extern "C"
JNIEXPORT void JNICALL
Java_com_test_a4over6_1vpn_MainActivity_disConnect(JNIEnv *env, jobject instance) {

    // TODO Disconnect
    disConnect();
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

    struct sockaddr_in6 server;
    bzero(&server, sizeof(server)); //把一段内存区的内容全部设置为0
    server.sin6_family = AF_INET6; //internet协议族
    server.sin6_port = htons(45000); //0表示让系统自动分配一个空闲端口
//    server.sin6_port = htons(9734); //0表示让系统自动分配一个空闲端口

    if (inet_pton(AF_INET6, "2402:f000:ffff:64:d454:4d7a:e00:a750", &server.sin6_addr) < 0)
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
        disConnect();
        LOGD("%s",strerror(errno));
        returnValue += ("connet to server error\n");
        return env->NewStringUTF(returnValue.c_str());
    }
    socketLive = true;
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