#ifndef __TALK_CLIENT_H__
#define __TALK_CLIENT_H__

#include <stdio.h>

#include <winsock2.h>
#include <Windows.h> 
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#include <thread>

#include <string>
#include <iostream>

#define PORT		12345 
#define	DATA_SIZE	87380

using namespace std;

class TalkClient
{
public:
	TalkClient(const string& host = "118.144.92.212", const int& post = 12345);
	~TalkClient();

private:
	void send_data();
	void recv_data();
	void send_data_size(const string& data);

	SOCKET m_server_sockfd;

	std::thread m_thrd;
};

#endif