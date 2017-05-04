#include "server.h"
#define MAXBUF 1024
#define ADDR_SIZE 128
#define IP_REQUEST 100
#define IP_RESPONCE 101
#define IT_REQUEST 102
#define IT_RESPONCE 103
#define HEARTBEAT 104
struct IPADDR ipaddr[ADDR_SIZE];//全局变量的地址池
User_Manager manager;
fd_set readfds, testfds;//select集合
static void *keepalive(void* args);
void send_ip_responce(int sockfd,int addr);
void send_104_package(int sockfd);
int handle_100(int sockfd);
int handle_104(int sockfd);
char dns_addr[] = "202.38.120.242 8.8.8.8 202.106.0.20";
char ip_base[] = "13.8.0.";
int main()
{
	//TODO:创建IPV6套接字，把该套接字加入Select模型字符集;
	int server_sockfd, client_sockfd, tun_fd;
	socklen_t server_len, client_len;
	struct sockaddr_in6 server_address;
	struct sockaddr_in6 client_address;
	int result;
	char buff[MAXBUF];

	server_sockfd = socket(AF_INET6, SOCK_STREAM, 0);//服务器监听socket
	memset(&server_address,0,sizeof(server_address));
	server_address.sin6_family = AF_INET6;
	server_address.sin6_addr = in6addr_any;
	server_address.sin6_port = htons(9734);
	server_len = sizeof(server_address);
	
	bind(server_sockfd,(struct sockaddr *)&server_address,server_len);
	
	listen(server_sockfd, 5);
	//初始化select集合
	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds);
	//TODO:创建tun虚接口
	tun_fd = socket(AF_INET, SOCK_STREAM, 0);//tun 虚接口
	//TODO:创建客户信息表和地址池
	for(int i=0; i<ADDR_SIZE; i++){
		ipaddr[i].status = 0;
		sprintf(ipaddr[i].addr,"%s%d",ip_base,i);
	}
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
	//pthread_create(&tun_id, NULL, &readtun, NULL);
	//TODO：主进程中while循环中数据处理。
	while(1){
		char ch;
		int fd;
		int nread;
		testfds = readfds;
		//printf("server waiting\n");
		result = select(FD_SETSIZE, &testfds, (fd_set *)0,(fd_set *)0,(struct timeval *)0);//非阻塞
		if (result < 1)
		{
			perror("server5");
			exit(1);
		}

		for(fd = 0; fd < FD_SETSIZE; fd++){
			if(FD_ISSET(fd,&testfds)){
				if(fd == server_sockfd){
					printf("server_fd\n");
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
                    temp.count = 20;
                    temp.secs = time(NULL);
                    memset(&temp.v4addr,0,sizeof(temp.v4addr));
                    manager.add_user(temp);
				}else{
					//data request
					//printf("datarequest\n");
					ioctl(fd, FIONREAD, &nread);//获取接收缓冲区的字节数
					if (nread!=0){
						Msg msg;
						read(fd,&msg,sizeof(msg));
						switch (msg.type){
							case IP_REQUEST :
								//TODO:回应101报文
								handle_100(fd);
								break;
							case 101 :
								break;
							case HEARTBEAT :
								//TODO:keepalive消息
								handle_104(fd);
								break;
							default :
								break;

						}
					}
				}
			}
		}
	}
	return 0;
}

static void *keepalive(void* args){
	pthread_detach(pthread_self());
	while(1){
		sleep(1000);
		User_Info_Table *current = manager.head;
		while(current = current->pNext){
			current -> count --;
			int fd = current->fd;
			if(current -> count == 0){
				send_104_package(fd);
				current -> count = 20;
			}
			unsigned int now = time(NULL);
			if((now - current->secs) > 60){
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


void send_ip_responce(int sockfd,int addr){
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
	int addr = 0;
	char ip[20];
	for(int addr=0;addr<ADDR_SIZE;addr++){
		if(ipaddr[addr].status == 0){
				ipaddr[addr].status = 1;
				break;
		}
	}
	if(addr != 128){
		send_ip_responce(sockfd,addr);
	}
	User_Info_Table *user = manager.find_user(sockfd);
	inet_pton(AF_INET, ipaddr[addr].addr, (void *)&user->v4addr);
	return 0;
}

int handle_104(int sockfd){
	User_Info_Table *user = manager.find_user(sockfd);
	user -> secs = time(NULL);
}

void send_104_package(int sockfd){
	Msg msg;
	msg.type = HEARTBEAT;
	memset(msg.data,0, sizeof(msg.data));
	msg.length = sizeof(msg);
	write(sockfd,&msg,msg.length);
}

void User_Manager::add_user(User_Info_Table user){
	User_Info_Table temp = user;
	User_Info_Table *current = head;
	while(current->pNext != tail){
		current = current->pNext;
	}
	temp.pNext = tail;
	current->pNext = &temp;
	return;
}

void User_Manager::del_user(int sockfd){
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
	User_Info_Table *current = head;
	while(current = current->pNext){
		if(current -> fd == sockfd){
			return current;
		}
	}
	return NULL;
}

User_Manager::User_Manager(){
	head = new User_Info_Table;
	head -> pNext = tail;
	tail = NULL;
}
