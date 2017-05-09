#include "server.h"


//int main(int argc, char *argv[])
//{
//    int tun, ret;
//    char tun_name[IFNAMSIZ];
//    unsigned char buf[4096];
//    unsigned char ip[4];
//
//    tun_name[0] = '/0';
//    tun = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
//    if (tun < 0)
//    {
//        return 1;
//    }
//    printf("TUN name is %s\n", tun_name);
//
//    //激活虚拟网卡增加到虚拟网卡的路由
//    interface_up(tun_name);
//    route_add(tun_name);
//
//    while (1) {
//
//        ret = read(tun, buf, sizeof(buf));
//        printf("read %d bytes\n", ret);
//        int i;
//        for(i=0;i<ret;i++)
//        {
//            printf("%02x ",buf[i]);
//        }
//        printf("\n");
//        if (ret < 0)
//            break;
//        memcpy(ip, &buf[12], 4);
//        memcpy(&buf[12], &buf[16], 4);
//        memcpy(&buf[16], ip, 4);
//        buf[20] = 0;
//        *((unsigned short*)&buf[22]) += 8;
//        ret = write(tun, buf, ret);
//        printf("write %d bytes\n", ret);
//    }
//
//    return 0;
//}