#include "server.h"

struct IPADDR ipaddr[ADDR_SIZE];//全局变量的地址池
int tun_fd;
fd_set readfds, testfds;//select集合
int ret;
char tun_name[IFNAMSIZ];
unsigned char buf[4096];
User_Manager manager;
int main() {
//创建IPV6套接字，把该套接字加入Select模型字符集;
    int server_sockfd, client_sockfd;
    socklen_t server_len, client_len, tun_len;
    struct sockaddr_in6 server_address;
    struct sockaddr_in6 client_address;
    int result;
    char buff[MAXBUF];
//服务器监听socket
    server_sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    memset(&server_address,0,sizeof(server_address));
    server_address.sin6_family = AF_INET6;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_port = htons(9734);
    server_len = sizeof(server_address);
    int on = 1;
    if (setsockopt(server_sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0)
    {
        perror("setsockopt");
        return -1;
    }
    if(bind(server_sockfd,(struct sockaddr *)&server_address,server_len)<0){
        printf("server bind failed, errno : %d",errno);
    }
    listen(server_sockfd, 10);
//初始化select集合
    FD_ZERO(&readfds);
    FD_SET(server_sockfd, &readfds);
//创建tun虚接口
    tun_name[0] = '\0';
    tun_fd = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
    if (tun_fd < 0)
    {
        printf("tun_creat failed!");
        return 1;
    }
    printf("TUN name is %s\n", tun_name);

//激活虚拟网卡增加到虚拟网卡的路由
    interface_up(tun_name);
    route_add(tun_name);
    char command[64];
    sprintf(command,"ifconfig %s 13.8.0.1/24",tun_name);
    system(command);
//创建客户信息表和地址池
    for(int i=0; i<ADDR_SIZE; i++){
        ipaddr[i].status = 0;
        sprintf(ipaddr[i].addr,"%s%d",ip_base,i+1);
    }
    ipaddr[0].status = 1;
//获取服务器DNS地址
    FILE *fp;
    system("cat /etc/resolv.conf | grep -i nameserver | cut -c 12-30 > ./dns.txt");
    char dns[32];
    if(fp = fopen("dns.txt","r")){
        fscanf(fp,"%s",dns);
    }else{
        perror("open dns.txt failed");
    }
    fclose(fp);
    in_addr server_v4;
    inet_pton(AF_INET, dns, (void *)&server_v4);
//创建keeplive线程
    pthread_t ka_id;
    pthread_create(&ka_id, NULL, keepalive, NULL);
//创建读取虚接口线程
    pthread_t tun_id;
    pthread_create(&tun_id, NULL, read_tun, NULL);
//主进程中while循环中数据处理。
    while(1){
        char ch;
        int fd;
        int nread;
        testfds = readfds;
        result = select(FD_SETSIZE, &testfds, (fd_set *)0,(fd_set *)0,(struct timeval *)0);//非阻塞
        if (result < 1)
        {
            printf("select result: %d  errno:%d\n",result,errno);
            exit(1);
        }
        for(fd = 0; fd < FD_SETSIZE; fd++){
            if(FD_ISSET(fd,&testfds)){
                if(fd == server_sockfd){
                    printf("connect request:\n");
                    memset(&client_address,0,sizeof(client_address));
                    client_len = sizeof(client_address);
                    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
                    FD_SET(client_sockfd, &readfds);//将客户端socket加入到集合中
                    printf("adding client on fd %d\n", client_sockfd);
                    printf("connection from %s, port %d\n", inet_ntop(AF_INET6,&client_address.sin6_addr,buff,sizeof(buff)),ntohs(client_address.sin6_port));
                    User_Info_Table temp;
                    temp.fd = client_sockfd;
                    temp.v6addr = client_address.sin6_addr;
                    temp.count = HBCOUNT;
                    temp.secs = time(NULL);
                    memset(&temp.v4addr,0,sizeof(temp.v4addr));
                    manager.add_user(temp);
                }else{
                    ioctl(fd, FIONREAD, &nread);//获取接收缓冲区的字节数
                    if (nread!=0){
                        Msg msg;
                        read(fd,&msg,sizeof(msg));
                        //printf("recv msg type %d\n", (int)msg.type);
                        switch (msg.type){
                            case IP_REQUEST :
                                //回应101报文
                                handle_100(fd);
                                break;
                            case 102 :
                                //上网请求消息
                                handle_102(fd,msg);
                                break;
                            case HEARTBEAT :
                                //keepalive消息
                                handle_104(fd);
                                break;
                            default :
                                //printf("unknown message type\n");
                                break;

                        }
                    }
                }
            }
        }
    }
    return 0;
}

void *keepalive(void* args){
    pthread_detach(pthread_self());
    printf("----keepalive thread start-----\n");
    while(1){
        //printf("----keepalive msg start-----\n");
        usleep(1000000);
        User_Info_Table *current = manager.head;
        while(current = current->pNext){
            current -> count --;
            int fd = current->fd;
            if(current -> count == 0){
                send_104_package(fd);
                current -> count = HBCOUNT;
            }
            unsigned int now = time(NULL);
            if((now - current->secs) > 60){
                printf("%d socket don't alive !\n",current->fd);
                in_addr v4addr = current->v4addr;
                for(int i=0; i<ADDR_SIZE; i++){
                    char addr[32];
                    if(strcmp(ipaddr[i].addr,inet_ntop(AF_INET, &v4addr, addr, sizeof(addr)))==0){
                        ipaddr[i].status = 0;
                        break;
                    }
                }
                FD_CLR(fd,&testfds);
                manager.del_user(fd);
                close(fd);
            }
        }
    }
    return 0;
}

void *read_tun(void* args){
    printf("----read tun thread start-----\n");
    Msg msg;
    int ret;
    while(1){
        ret = read(tun_fd, buf, sizeof(buf));
        if(ret>0) {
            //printf("recv from tun %d bytes: errno: %d\n",num,errno);
            IP_HEAD iphead;
            iphead = *(IP_HEAD*)buf;

            in_addr dest = *(in_addr*)&iphead.dwIPDes;
            in_addr sour = *(in_addr*)&iphead.dwIPSrc;
            char buff[32];
            if(memcmp("13.8.0",inet_ntop(AF_INET,&dest,buff,sizeof(buff)),6)==0){
                printf("read from tun %d bytes",ret);
                printf("recv source addr: %s\n",inet_ntop(AF_INET,&sour,buff,sizeof(buff)));
                printf("recv dest addr: %s\n",inet_ntop(AF_INET,&dest,buff,sizeof(buff)));
                in_addr temp;
                int length = ntohs(iphead.wPacketLen);
                //printf("here\n");
                inet_pton(AF_INET, ipaddr[1].addr, (void *)&temp);
                memcpy(buff+16,&temp,4);
                //assert(length<4096);
                memcpy(msg.data,buff,length);
                msg.length = length + 5;
                msg.type = 103;
                //printf("here\n");
                User_Info_Table *user = manager.find_user(temp);
                if(user){
                    int fd = user->fd;
                    int num = write(fd,&msg,msg.length);
                    printf("send 103 message, length: %d",msg.length);
                }
            }
        }
    }
    return 0;
}

void send_ip_responce(int sockfd,int addr){
    printf("----send_ip_responce-----\n");
    char data[1024];
    memset(data,0, sizeof(data));
    sprintf(data,"%s 0.0.0.0 %s",ipaddr[addr].addr,dns_addr);
    Msg msg;
    msg.type = 102;
    memset(msg.data,0, sizeof(msg.data));
    memcpy(msg.data,data,sizeof(data));
    msg.length = 5 + sizeof(data);
    int num = write(sockfd,(char *)&msg,sizeof(msg));
    printf("101 data : %s \n",msg.data);
    printf("send_101_message %d bytes from %d\n",num,sockfd);
    if(num < 0){
        printf("send_101_message failed! errno : %d\n ",errno);
    }
}

int handle_100(int sockfd){
    //printf("----handle  100-----\n");
    int addr = 0;
    char ip[20];
    for(addr=0;addr<ADDR_SIZE;addr++){
        if(ipaddr[addr].status == 0){
            ipaddr[addr].status = 1;
            break;
        }
    }
    printf("alloc : addr %s\n",ipaddr[addr].addr);
    if(addr != 128){
        send_ip_responce(sockfd,addr);
    }
    User_Info_Table *user = manager.find_user(sockfd);
    inet_pton(AF_INET, ipaddr[addr].addr, (void *)&user->v4addr);
    return 0;
}

int handle_104(int sockfd){
    //printf("----handle  104----- : %d\n",sockfd);
    User_Info_Table *user = manager.find_user(sockfd);
    user -> secs = time(NULL);
    return 0;
}

void send_104_package(int sockfd){
    printf("send message  104 to %d\n",sockfd);
    Msg msg;
    msg.type = HEARTBEAT;
    memset(msg.data,0, sizeof(msg.data));
    msg.length = sizeof(msg);
    int num = write(sockfd,&msg,msg.length);
    printf("write to %d  %d bytes : msg.type: %d\n",sockfd,num,(int)msg.type);
}

void handle_102(int fd,Msg msg){
    printf("102 Message -> data: %s\n",(char *)ntohs(*(unsigned int*)msg.data));
    int length = msg.length - 5;
    //printf("length : %d", length);
    char buff[32];
    char data[4096];
    memcpy(data,msg.data,sizeof(msg.data));

//	User_Info_Table *user = manager.find_user(fd);
//	in_addr source = user->v4addr;
//	memcpy(data+12,(char *)&source,4);//该源地址

    IP_HEAD iphead;
    iphead = *(IP_HEAD*)data;

    in_addr dest = *(in_addr*)&iphead.dwIPDes;
//	in_addr sour = *(in_addr*)&iphead.dwIPSrc;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr = dest; //IP
    addr.sin_port = 8888; //端口，IP层端口可随意填
//    char ptype = iphead.byProtocolType;
//    char *neir = data + sizeof(struct IP_HEAD) + sizeof(struct TCP_HEAD);
//    printf("length: %u\n",ntohs(iphead.wPacketLen));
//    printf("request data : %s\n",neir);
//	printf("protocol type %c,   %d\n",ptype,(int)ptype);
//	printf("source addr: %s\n",inet_ntop(AF_INET,&sour,buff,sizeof(buff)));
//    printf("source port: %d\n",tcphead.m_sSourPort);
//    printf("dest port: %d\n",tcphead.m_sDestPort);
	printf("dest addr: %s\n",inet_ntop(AF_INET,&addr.sin_addr,buff,sizeof(buff)));
    int num = sendto(tun_fd, data, length, 0, (struct sockaddr * )&addr, sizeof(addr));
    printf("send to tun  %d bytes, errno : %d\n",num,errno);
}
