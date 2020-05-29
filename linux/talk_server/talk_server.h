#ifndef __TALK_SERVER_H__
#define __TALK_SERVER_H__
#include <stdio.h>
#include <string.h>		//bzero

#include <sys/socket.h>	//socket bind listen accept
#include <arpa/inet.h>	//hton
#include <unistd.h>		//read write sleep

#include <sys/ioctl.h>	//ioctl …Ë÷√∑«◊Ë»˚
#include <sys/epoll.h>	//epoll

#include <errno.h>
#include <thread>

#include <iostream>
#include <map>

#include "wrap.h"
#include "user.h"
using namespace std;

class TalkServer
{
#define	POST		12345
#define	DATA_SIZE	87380
public:
	TalkServer();
	~TalkServer();

	void init();
	void run();

private:
	void run_accept();
	void run_epoll();

	void data_center(const int& counter_fd, const string& readbuf);

	bool m_init;
	bool m_stop;
	std::thread m_thrd;

	int m_server_sockfd;
	int m_epoll_sockfd;

	map<int, User> m_users;
	
};

#endif