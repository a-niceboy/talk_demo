#ifndef __WRAP_H__
#define __WRAP_H__

#include <sys/socket.h>	//socket bind listen accept
#include <arpa/inet.h>	//hton
#include <unistd.h>		//read write sleep

#include <sys/ioctl.h>	//ioctl 设置非阻塞
#include <sys/epoll.h>	//epoll

#include <string.h>		//bzero
#include <errno.h>
#include <iostream>
using namespace std;

//创建服务器fd
int Socket(int domain, int type, int protocol);
// 设置 地址复用
int Setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
//设置非阻塞-------放在绑定之前
int Ioctl(int d, int request, int* mode);

int Bind(int sockfd, int domain, const int& post, in_addr_t s_addr);

int Listen(int sockfd, int backlog);

int Accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);

int Epoll_create(int size);

int Epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
#endif