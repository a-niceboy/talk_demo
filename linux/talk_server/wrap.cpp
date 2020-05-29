#include "wrap.h"

int Socket(int domain, int type, int protocol)
{
	int ret = 0;
	ret = socket(domain, type, protocol);
	if (ret == -1)
	{
		cerr << "m_server_sockfd make error." << endl;
	}

	return ret;
}

int Setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
{
	int ret = 0;
	ret = setsockopt(sockfd, level, optname, optval, optlen);
	if (ret < 0)
	{
		cerr << "setsockopt error." << endl;
	}

	return ret;
}

int Ioctl(int d, int request, int * mode) 
{
	int ret = 0;

	ret = ioctl(d, request, mode);
	if (ret)
	{
		cerr << "ioctl to no block error." << endl;
	}
	return ret;
}

int Bind(int sockfd, int domain, const int& post, in_addr_t s_addr)
{
	int ret = 0;

	//°ó¶¨ÐÅÏ¢
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));

	addr.sin_family = domain;
	addr.sin_port = htons(post);
	addr.sin_addr.s_addr = s_addr;

	ret = bind(sockfd, (sockaddr*)&addr, sizeof(addr));
	if (ret)
	{
		cerr << "bind error." << endl;
	}
	return ret;
}

int Listen(int sockfd, int backlog)
{
	int ret = 0;
	ret = listen(sockfd, backlog);
	if (ret)
	{
		cerr << "listen error." << endl;
	}
	return ret;
}

int Accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
	int ret = 0;
	ret = accept(sockfd, addr, addrlen);
	if (ret < 0)
	{
		cerr << "accept error." << endl;
	}
	return ret;
}

int Epoll_create(int size)
{
	int ret = 0;
	ret = epoll_create(size);
	if (ret < 0)
	{
		cerr << "epoll create error." << endl;
	}
	return ret;
}

int Epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) 
{
	int ret = 0;
	ret = Epoll_ctl(epfd, op, fd, event);
	if (ret < 0)
	{
		cerr << "epoll add error." << endl;
	}
	return ret;
}