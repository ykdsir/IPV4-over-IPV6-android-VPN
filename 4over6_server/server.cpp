#include "server.h"
#define MAXBUF 1024
#define ADDR_SIZE 128 //address area size.
#define HBCOUNT 20 //Heart Beat initial count.
#define IP_REQUEST 100
#define IP_RESPONCE 101
#define IT_REQUEST 102
#define IT_RESPONCE 103
#define HEARTBEAT 104
struct IPADDR ipaddr[ADDR_SIZE];//全局变量的地址池
User_Manager manager;
fd_set readfds, testfds;//select集合
int tun_fd;
int ret;
char tun_name[IFNAMSIZ];
//------------
int res = 0;//剩余的包的长度
int resLength = 0;//结束时未收完的下一个包的长度
char pre[4096];
char recvBuff[MAXBUFF];

//-------------
//unsigned char buf[4096];
static void *keepalive(void* args);
static void *read_tun(void* args);
void send_ip_responce(int sockfd,int addr);
void send_104_package(int sockfd);
int handle_100(int sockfd);
int handle_104(int sockfd);
void handle_102(int fd,Msg msg);
int recv_Msg(int fd, Msg *msg);
void handleMsg(int fd,Msg* msg);
void free_inaddr(in_addr v4addr);
char dns_addr[] = "202.38.120.242 8.8.8.8 202.106.0.20";
char ip_base[] = "13.8.0.";
int main()
{
	//TODO:创建IPV6套接字，把该套接字加入Select模型字符集;
	int server_sockfd, client_sockfd;
	socklen_t server_len, client_len, tun_len;
	struct sockaddr_in6 server_address;
	struct sockaddr_in6 client_address;
	struct sockaddr_in tun_address;
	int result;
	char buff[MAXBUF];

	server_sockfd = socket(AF_INET6, SOCK_STREAM, 0);//服务器监听socket
	memset(&server_address,0,sizeof(server_address));
	server_address.sin6_family = AF_INET6;
	server_address.sin6_addr = in6addr_any;
	server_address.sin6_port = htons(45000);
	server_len = sizeof(server_address);
	
	int on1 = 1;
    if (setsockopt(server_sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &on1, sizeof(on1)) < 0)
    {
        perror("setsockopt");
        return -1;
    }
        if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &on1, sizeof(on1)) < 0)
    {
        perror("setsockopt");
        return -1;
    }
    if(bind(server_sockfd,(struct sockaddr *)&server_address,server_len)<0){
        printf("server bind failed, errno : %d",errno);
    }
	
	if(listen(server_sockfd, 10) < 0) {
		printf("server listen failed , errno%d\n", errno);
	}
	//初始化select集合
	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds);
	//TODO:创建tun虚接口
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
    sprintf(command,"route add -net 13.8.0.0 netmask 255.255.255.0 dev %s",tun_name);
    system(command);
    sprintf(command,"bash -c 'echo 1 > /proc/sys/net/ipv4/ip_forward'");
    system(command);
    sprintf(command,"sh iptables.sh");
    system(command);

	//TODO:创建客户信息表和地址池
	for(int i=0; i<ADDR_SIZE; i++){
		ipaddr[i].status = 0;
		sprintf(ipaddr[i].addr,"%s%d",ip_base,i+1);
	}
	ipaddr[0].status = 1;
	//TODO:创建keeplive线程
	pthread_t ka_id;
	pthread_create(&ka_id, NULL, keepalive, NULL);
	//TODO:创建读取虚接口线程
	pthread_t tun_id;
	pthread_create(&tun_id, NULL, read_tun, NULL);
	//TODO：主进程中while循环中数据处理。

	struct sigaction sa;
	sa.sa_handler = SIG_IGN;//设定接受到指定信号后的动作为忽略
	sa.sa_flags = 0;
	if (sigemptyset(&sa.sa_mask) == -1 ||   //初始化信号集为空
		sigaction(SIGPIPE, &sa, 0) == -1) {   //屏蔽SIGPIPE信号
		perror("failed to ignore SIGPIPE; sigaction");
		exit(EXIT_FAILURE);
	}
    int temp100 = 0;
	while(1){
		char ch;
		int fd;
		int nread = 0;
		testfds = readfds;
		struct stat tStat;
		for(fd = 0; fd < FD_SETSIZE; fd++) {
			if (FD_ISSET(fd,&testfds) && (-1 == fstat(fd, &tStat))) {
				printf("fstat %d error:%s\n", fd, strerror(errno));
				FD_CLR(fd, &readfds);
			}
		}
		result = select(FD_SETSIZE, &testfds, (fd_set *)0,(fd_set *)0,(struct timeval *)NULL);//非阻塞
		if (result < 1)
		{
			printf("select result : %d, errno: %s\n",result,strerror(errno));
			continue;
			//exit(1);
		}
		for(fd = 0; fd < FD_SETSIZE; fd++){
			if(FD_ISSET(fd,&testfds)){
				if(fd == server_sockfd){
					printf("connect request:\n");
					//connect request ----- put client into set
					memset(&client_address,0,sizeof(client_address));
					client_len = sizeof(client_address);   
                    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len); 
                    if(client_sockfd<0){
                    	printf("client accept failed, errno : %s\n", strerror(errno));
                    }
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
                    printf("after add user\n");
				}else{
					ioctl(fd, FIONREAD, &nread);//获取接收缓冲区的字节数
					if (nread>4){
						Msg* msg = new Msg();
                        //printf("nread >= 4: %d\n",nread);
						int num = 0;
                        //num = recv(fd,&msg,sizeof(msg),0);
                        num = recv_Msg(fd,msg);
//                        printf("end recive\n");
                        if(num<msg->length){
                            printf("num < length : %d < %d\n",num,msg->length);
                            continue;
                        }
                        handleMsg(fd,msg);
					}
				}
			}
		}
	}
	//pthread_join(ka_id, NULL);
	//pthread_join(tun_id, NULL);
	return 0;
}

void handleMsg(int fd,Msg* msg){
    switch (msg->type){
        case IP_REQUEST :
            //TODO:回应101报文
            handle_100(fd);
            break;
        case 102 :
            //TODO:上网请求消息
            handle_102(fd,*msg);
            break;
        case HEARTBEAT :
            //TODO:keepalive消息
            handle_104(fd);
            break;
        default :
            break;
    }
}

static void *keepalive(void* args){
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
				free_inaddr(v4addr);
				FD_CLR(fd,&readfds);
				manager.del_user(fd);
				try{
					close(fd);
				}catch (...){
					printf("close fd failed ");
				}
			}
		}
	}
	return 0;
}


void free_inaddr(in_addr v4addr){
	for(int i=0; i<ADDR_SIZE; i++){
		char addr[32];
		if(strcmp(ipaddr[i].addr,inet_ntop(AF_INET, &v4addr, addr, sizeof(addr)))==0){
			ipaddr[i].status = 0;
			break;
		}
	}
}

static void *read_tun(void* args){
	printf("----read tun thread start-----\n");
    Msg msg;
    int ret = 0;
    unsigned char buf[65535];
    while(1){
        ret = read(tun_fd, buf,sizeof(buf));
		if(ret>0) {
            IP_HEAD iphead;
            iphead = *(IP_HEAD*)buf;

            in_addr dest = *(in_addr*)&iphead.dwIPDes;
            in_addr sour = *(in_addr*)&iphead.dwIPSrc;

            char buff[32];
            //printf("103 dest addr : %s\n",inet_ntop(AF_INET,&dest,buff,sizeof(buff)));
            if(memcmp("13.8.0",inet_ntop(AF_INET,&dest,buff,sizeof(buff)),6)==0){
                in_addr temp;
                int length = ntohs(iphead.wPacketLen);
                memcpy(msg.data,buf,length);
                if(length > 4096)
	                printf("one ip package length: %d\n",length );
                msg.length = length + 5;
                msg.type = 103;
                User_Info_Table *user = manager.find_user(dest);
                if(user){
                    int fd = user->fd;
                    if(!FD_ISSET(fd,&testfds))continue;
					int num = 0;
                    try{
                    	num = write(fd,&msg,msg.length);
                        if(num < msg.length){
                            printf("103msg is not complete %d / %d",num,msg.length);
                        }
                    }catch(...){
						printf("write 103 message to %d failed, errno : %s\n",fd, strerror(errno));
                    }
					if(num<0){
                        if(errno != EINTR && errno != 0){
                            printf("send 103 message failed, errno : %s\n", strerror(errno));
                            FD_CLR(fd,&readfds);
                            manager.del_user(fd);
                            free_inaddr(dest);
                            try{
                                close(fd);
                                continue;
                            }catch (...){
                                printf("main close fd failed \n");
                            }
                        }
					}
                    printf("send 103 message to %d, length: %d\n",fd,num);
                }
            }
       }else{
			printf("tun_fd ret < 0 errno: %s\n",strerror(errno));
		}
    }
    return 0;
}

void send_ip_responce(int sockfd,int addr){
	//printf("----send_ip_responce-----\n");
	char data[4096];
	memset(data,0, sizeof(data));
	sprintf(data,"%s 0.0.0.0 %s",ipaddr[addr].addr,dns_addr);
	Msg msg;
	msg.type = IP_RESPONCE;
	memset(msg.data,0, sizeof(msg.data));
	memcpy(msg.data,data,200);
	printf("data size :%d\n", sizeof(data));
	msg.length = 200;
	printf("send msg : type: %c, length: %d, data: %s\n",msg.type,msg.length,msg.data );
	int num = 0;
	try {
		num = write(sockfd,&msg,msg.length);
	}catch(...) {
		printf("send ip responce failed, errno : %s\n", strerror(errno));
	}
	printf("send 101 message %d bytes to %d\n", num,sockfd);
	return;
}

int handle_100(int sockfd){
	printf("handle  100 message from %d\n",sockfd);
	int addr = 0;
	char ip[20];
	for(addr=0;addr<ADDR_SIZE;addr++){
		if(ipaddr[addr].status == 0){
				ipaddr[addr].status = 1;
				break;
		}
	}
	if(addr != 128){
        printf("alloc : addr %s\n",ipaddr[addr].addr);
		send_ip_responce(sockfd,addr);
	}
	User_Info_Table *user = manager.find_user(sockfd);
	inet_pton(AF_INET, ipaddr[addr].addr, (void *)&user->v4addr);
	return 0;
}

int handle_104(int sockfd){
	printf("handle  104 message from %d\n",sockfd);
	User_Info_Table *user = manager.find_user(sockfd);
	user -> secs = time(NULL);
	return 0;
}

void send_104_package(int sockfd){
	printf("send message  104 to %d\n",sockfd);
	Msg msg;
	msg.type = HEARTBEAT;
	memset(msg.data,0, sizeof(msg.data));
	msg.length = 200;

	try {
		int num = write(sockfd,&msg,msg.length);
	}catch(...) {
		printf("send 104 message failed errno : %d\n", errno);
	}
	//printf("write to %d  %d bytes : msg.type: %d\n",sockfd,num,(int)msg.type);
	return;
}

void handle_102(int fd,Msg msg){
	
	char buff[32];
	char data[4096];

	memcpy(data,msg.data,sizeof(msg.data));

	 IP_HEAD iphead;
	 iphead = *(IP_HEAD*)data;
	 in_addr dest = *(in_addr*)&iphead.dwIPDes;
	 in_addr sour = *(in_addr*)&iphead.dwIPSrc;
	int length = ntohs(iphead.wPacketLen);

    int num = 0 ;
	try {
		num = write(tun_fd, data, length);
	}catch(...) {
		printf("tun_fd : %d ", tun_fd);
		printf("send  from tun failed errno : %s\n", strerror(errno));
	}
	//printf("send to tun  %d bytes\n",num);
	char bufff[32];
	printf("hangle 102 message from :%s , to %s\n", inet_ntop(AF_INET,&sour,bufff,sizeof(bufff)),inet_ntop(AF_INET,&dest,buff,sizeof(buff)));
	return;
}

void User_Manager::add_user(User_Info_Table user){
	printf("User_Manager::add_user\n");
	User_Info_Table *temp = new User_Info_Table(user);
	User_Info_Table *current = head;
	while(current->pNext != tail){
		current = current->pNext;
	}
	temp->pNext = tail;
	current->pNext = temp;
	return;
}

void User_Manager::del_user(int sockfd){
	printf("User_Manager::del_user: %d\n",sockfd);
	User_Info_Table *user = this->find_user(sockfd);
	if(user == NULL){
		printf("del user failed %s \n", strerror(errno));
		return;
	}
	User_Info_Table *current = head;
	while(current->pNext != tail){
		if(current->pNext == user){
			current->pNext = user->pNext;
			break;
		}
	}
	return;
}


int recv_Msg(int fd, Msg *reciveMsg){
    int num = 0;
    try {
        while(num<4){
            num = recv(fd,reciveMsg,4,MSG_PEEK);
            if(num==0){
                printf("recv_Msg from %d error : %s,\n", fd, strerror(errno));
            }
            if(num<0){
                if(errno != EINTR && errno != 0){
                    printf("recv_Msg from %d failed errno : %s, \n" , fd,strerror(errno));
                    FD_CLR(fd,&readfds);
                    User_Info_Table *user = manager.find_user(fd);
                    in_addr v4addr = user->v4addr;
                    free_inaddr(v4addr);
                    manager.del_user(fd);
                    try{
                        close(fd);
                        return -1;
                    }catch (...){
                        printf("main close fd failed \n");
                    }
                }
            }
        }
        int length = reciveMsg->length;
        num = 0;
        while(num < length){
            int temp = recv(fd,reciveMsg+num,length-num,MSG_WAITALL);
            if(temp > 0){
                num += temp;
            }else{
                printf("temp < 0: num:%d / length:%d error : %s\n", num,length, strerror(errno));
                break;
            }
            //num += recv(fd,reciveMsg+num,length-num,MSG_WAITALL);
        }
        //num = recv(fd,msg,length,MSG_WAITALL);
        return num;
    }catch (...) {
        printf("recv_Msg from %d error : %s", fd, strerror(errno));
        return -1;
    }
}

User_Info_Table *User_Manager::find_user(int sockfd){
	//printf("*User_Manager::find_user : %d\n",sockfd);
	User_Info_Table *current = head;
	while(current->pNext){
		current = current->pNext;
		//printf("check user fd : %d\n",current -> fd);
		if(current -> fd == sockfd){
			return current;
		}
	}
	printf("*User_Manager::find_user from fd failed\n");
	return NULL;
}

User_Manager::User_Manager(){
	printf("User_Manager::User_Manager\n");
	head = new User_Info_Table;
	tail = NULL;
	head -> pNext = tail;
	return;
}

User_Info_Table *User_Manager::find_user(in_addr v4addr){
    User_Info_Table *current = head;
    while(current->pNext){
        current = current->pNext;
        //printf("check user fd : %d\n",current -> fd);
        if(memcmp(&current->v4addr,&v4addr,sizeof(struct in_addr))==0){
            //printf("*User_Manager::find_user successed \n");
            return current;
        }
    }
    char buff[32];
    //printf("*User_Manager::find_user %s failed\n",inet_ntop(AF_INET,&v4addr,buff,sizeof(buff)));
    return NULL;
}


/**
 *  激活接口
 */
int interface_up(char *interface_name)
{
    int s;

    if((s = socket(PF_INET,SOCK_STREAM,0)) < 0)
    {
        printf("Error create socket :%d\n", errno);
        return -1;
    }

    struct ifreq ifr;
    strcpy(ifr.ifr_name,interface_name);

    short flag;
    flag = IFF_UP;
    if(ioctl(s, SIOCGIFFLAGS, &ifr) < 0)
    {
        printf("Error up %s :%d\n",interface_name, errno);
        return -1;
    }

    ifr.ifr_ifru.ifru_flags |= flag;

    if(ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
    {
        printf("Error up %s :%d\n",interface_name, errno);
        return -1;
    }

    return 0;

}

/**
 *  设置接口ip地址
 */
int set_ipaddr(char *interface_name, char *ip)
{
    int s;

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error up %s :%d\n",interface_name, errno);
        return -1;
    }

    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface_name);

    //struct sockaddr_in genmask;
    //genmask.sin_addr.s_addr = inet_addr("255.255.255.255");

    struct sockaddr_in addr;
    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = PF_INET;
    inet_aton(ip, &addr.sin_addr);



    memcpy(&ifr.ifr_ifru.ifru_addr, &addr, sizeof(struct sockaddr_in));

    if(ioctl(s, SIOCSIFADDR, &ifr) < 0)
    {
        printf("Error set %s ip :%d\n",interface_name, errno);
        return -1;
    }

    return 0;
}

/**
 *  创建接口
 */
int tun_create(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        printf("Error :%d\n", errno);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags |= flags;

    if (*dev != '\0')
    {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
    {
        printf("Error :%d\n", errno);
        close(fd);
        return -1;
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}

/**
 *  增加到13.8.0.2的路由
 *  同命令:route add 10.0.0.1 dev tun0
 */
int route_add(char * interface_name)
{
    int skfd;
    struct rtentry rt;

    struct sockaddr_in dst;
    struct sockaddr_in gw;
    struct sockaddr_in genmask;

    memset(&rt, 0, sizeof(rt));

    genmask.sin_addr.s_addr = inet_addr("255.255.255.255");

    bzero(&dst,sizeof(struct sockaddr_in));
    dst.sin_family = PF_INET;
    dst.sin_addr.s_addr = inet_addr("13.8.0.1");

    rt.rt_metric = 0;
    rt.rt_dst = *(struct sockaddr*) &dst;
    rt.rt_genmask = *(struct sockaddr*) &genmask;

    rt.rt_dev = interface_name;
    rt.rt_flags = RTF_UP | RTF_HOST ;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(ioctl(skfd, SIOCADDRT, &rt) < 0)
    {
        printf("Error route add :%d\n", errno);
        return -1;
    }
	return 1;
}
