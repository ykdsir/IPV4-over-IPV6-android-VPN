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
static void *keepalive(void* args);
static void *read_tun(void* args);
void send_ip_responce(int sockfd,int addr);
void send_104_package(int sockfd);
int handle_100(int sockfd);
int handle_104(int sockfd);
void handle_102(int fd,Msg msg);
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
	server_address.sin6_port = htons(9734);
	server_len = sizeof(server_address);
	
	bind(server_sockfd,(struct sockaddr *)&server_address,server_len);
	
	listen(server_sockfd, 10);
	//初始化select集合
	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds);
	//TODO:创建tun虚接口
	tun_fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);//tun 虚接口
	printf("tun_fd : %d, errno : %d\n", tun_fd,errno);
	const int on=1;
	if(setsockopt(tun_fd,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on))<0){
		printf("tun init failed errno : %d\n",errno);
	}
	memset(&tun_address, 0, sizeof(tun_address));
	//tun_address.sin_family = AF_INET;
	//inet_pton(AF_INET, "13.0.8.1", (void *)&tun_address.sin_addr);
	//tun_address.sin_port = htons(9735);
	//bind(tun_fd,(struct sockaddr *)&tun_address, sizeof(tun_address));

	//TODO:创建客户信息表和地址池
	for(int i=0; i<ADDR_SIZE; i++){
		ipaddr[i].status = 0;
		sprintf(ipaddr[i].addr,"%s%d",ip_base,i+1);
	}
	ipaddr[0].status = 1;
	//TODO:获取服务器DNS地址

	FILE *fp; 
	system("cat /etc/resolv.conf | grep -i nameserver | cut -c 12-30 > ./dns.txt");
	char dns[32];
	if(fp = fopen("dns.txt","r")){
		fscanf(fp,"%s",dns);
	}else{
		perror("open dns.txt failed");
	}
	fclose(fp);
	printf("dns:%s\n",dns);
	in_addr server_v4;
	inet_pton(AF_INET, dns, (void *)&server_v4);
	//TODO:创建keeplive线程
	pthread_t ka_id;
	pthread_create(&ka_id, NULL, keepalive, NULL);
	//TODO:创建读取虚接口线程
	pthread_t tun_id;
	pthread_create(&tun_id, NULL, read_tun, NULL);
	//TODO：主进程中while循环中数据处理。

	while(1){
		char ch;
		int fd;
		int nread;
		testfds = readfds;
		//printf("server waiting\n");
		result = select(FD_SETSIZE, &testfds, (fd_set *)0,(fd_set *)0,(struct timeval *)NULL);//非阻塞
		if (result < 1)
		{
			printf("result: %d close!!!!!!  errno:%d\n",result,errno);
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
						//printf("recvmsg type : %d\n",(int)msg.type);
						switch (msg.type){
							case IP_REQUEST :
								//TODO:回应101报文
								handle_100(fd);
								break;
							case 102 :
								//TODO:上网请求消息
								handle_102(fd,msg);
								break;
							case HEARTBEAT :
								//TODO:keepalive消息
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
	pthread_join(ka_id, NULL);
	pthread_join(tun_id, NULL);
	return 0;
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

static void *read_tun(void* args){
    printf("----read tun thread start-----\n");
	while(1){
		char buff[4096];
		if(int num = recv(tun_fd,buff,sizeof(buff),0)>0) {
			//printf("recv from tun %d bytes: errno: %d\n",num,errno);
			IP_HEAD iphead;
			iphead = *(IP_HEAD*)buff;
			in_addr dest = *(in_addr*)&iphead.dwIPDes;
			in_addr sour = *(in_addr*)&iphead.dwIPSrc;
			char ptype = iphead.byProtocolType;
			//printf("recv protocol type %c,   %d\n",ptype,(int)ptype);
			//printf("recv source addr: %s\n",inet_ntop(AF_INET,&sour,buff,sizeof(buff)));
			if(memcmp("59.66.134.77",inet_ntop(AF_INET,&dest,buff,sizeof(buff)),12)){
				printf("recv dest addr: %s\n",inet_ntop(AF_INET,&dest,buff,sizeof(buff)));
			}


		}
	}
	return 0;
}

void send_ip_responce(int sockfd,int addr){
	printf("----send_ip_responce-----\n");
	char data[4096];
	memset(data,0, sizeof(data));
	sprintf(data,"%s 0.0.0.0 %s",ipaddr[addr].addr,dns_addr);
	Msg msg;
	msg.type = IP_RESPONCE;
	memset(msg.data,0, sizeof(msg.data));
	memcpy(msg.data,data,sizeof(data));
	msg.length = 5 + sizeof(data);
	write(sockfd,&msg,msg.length);
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
	//printf("write to %d  %d bytes : msg.type: %d\n",sockfd,num,(int)msg.type);
}

void handle_102(int fd,Msg msg){
	printf("102 Message -> data: %s\n",msg.data);
    int length = msg.length - 5;
    //printf("length : %d", length);
	char buff[32];
	char data[4096];
	printf("\n");
	memcpy(data,msg.data,sizeof(msg.data));
	User_Info_Table *user = manager.find_user(fd);
	in_addr source = user->v4addr;
	memcpy(data+12,(char *)&source,4);
	IP_HEAD iphead;
    TCP_HEAD tcphead;
	iphead = *(IP_HEAD*)data;
    tcphead = *(TCP_HEAD*)(data + sizeof(struct IP_HEAD));
	in_addr dest = *(in_addr*)&iphead.dwIPDes;
	in_addr sour = *(in_addr*)&iphead.dwIPSrc;
	char ptype = iphead.byProtocolType;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr = dest; //IP
	addr.sin_port = 8888; //端口，IP层端口可随意填
    char *neir = data + sizeof(struct IP_HEAD) + sizeof(struct TCP_HEAD);
//    printf("request data : %s\n",neir);
//	printf("protocol type %c,   %d\n",ptype,(int)ptype);
//	printf("source addr: %s\n",inet_ntop(AF_INET,&sour,buff,sizeof(buff)));
//    printf("source port: %d\n",tcphead.m_sSourPort);
//    printf("dest port: %d\n",tcphead.m_sDestPort);
//	printf("dest addr: %s\n",inet_ntop(AF_INET,&addr.sin_addr,buff,sizeof(buff)));
//	printf("tun_fd : %d ", tun_fd);

	int num = sendto(tun_fd, data, length, 0, (struct sockaddr * )&addr, sizeof(addr));
	printf("send to tun  %d bytes, errno : %d\n",num,errno);
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
	User_Info_Table *current = head;
	while(current->pNext != tail){
		if(current->pNext == user){
			current->pNext = user->pNext;
			break;
		}
	}
}

User_Info_Table *User_Manager::find_user(int sockfd){
	printf("*User_Manager::find_user : %d\n",sockfd);
	User_Info_Table *current = head;
	while(current->pNext){
		current = current->pNext;
		//printf("check user fd : %d\n",current -> fd);
		if(current -> fd == sockfd){
			return current;
		}
	}
	printf("*User_Manager::find_user failed\n");
	return NULL;
}

User_Manager::User_Manager(){
	printf("User_Manager::User_Manager\n");
	head = new User_Info_Table;
	tail = NULL;
	head -> pNext = tail;
}
