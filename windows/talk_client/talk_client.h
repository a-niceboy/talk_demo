#ifndef __TALK_CLIENT_H__
#define __TALK_CLIENT_H__

#include <stdio.h>

#include <winsock2.h>
#include <Windows.h> 
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#include <thread>
#include <queue>
#include <string>
#include <iostream>

#define PORT		6565 
#define	DATA_SIZE	16384//87380

using namespace std;

class TalkClient
{
public:
	TalkClient(const string& host = "118.144.92.212", const int& post = PORT);
	~TalkClient();
	void run();
private:
	void send_data();
	void recv_data();
	void draw_windows();
	void send_data_size(const string& data);

	int index = 0;

	const int BODY_HEIGHT = 10;
	const int INPUT_HEIGHT = 5;
	const int HEIGHT = 100;
	const int WEIGHT = 30;

	bool m_stop;
	SOCKET m_server_sockfd;

	std::thread m_thrd;
	string m_read_buf;
	queue<string> talk_datas;
};

#endif