#include "server.h"

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
    //printf("*User_Manager::find_user failed\n");
    return NULL;
}

User_Manager::User_Manager(){
	printf("User_Manager::User_Manager\n");
	head = new User_Info_Table;
	tail = NULL;
	head -> pNext = tail;
}

