#ifndef __TALK_CLIENT_H__
#define __TALK_CLIENT_H__

#include <stdio.h>

#include <winsock2.h>
#include <Windows.h> 
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#include <thread>
#include <deque>
#include <vector>
#include <string>
#include <iostream>

#define PORT		6565 
#define	DATA_SIZE	1500//87380

using namespace std;

class TalkClient
{
public:
	TalkClient(const string& host = "118.144.92.212", const int& post = PORT);
	~TalkClient();
	void init();
	void run();

private:
	void init_window();
	void init_socket();
	void init_ani_data();

	void begin_ani();

	void input_nick_name();

	void menu();
	void create_room();
	void join_room(const int& room_id = 0);

	void into_room_ani();

	void talking();
	void send_data();
	void recv_data();
	void draw_windows();

	void quilt_room();
	void quilt_room_ani();

	void quilt();
	void quilt_ani();	

	void send_data_size(const string& data);

	const int BODY_HEIGHT = 10;
	const int INPUT_HEIGHT = 5; 
	const int HEIGHT = 20;
	const int WEIGHT = 60;

	bool m_stop;
	SOCKET m_server_sockfd;
	string m_host;
	int m_post;

	int m_room_id;

	std::thread m_thrd;
	string m_read_buf;
	bool is_full;
	deque<string> talk_datas;

	/*
             __                     __       ____
 _    _____ / /______  __ _  ___   / /____ _/ / /__
| |/|/ / -_) / __/ _ \/  ' \/ -_) / __/ _ `/ /  '_/
|__,__/\__/_/\__/\___/_/_/_/\__/  \__/\_,_/_/_/\_\
*/
	vector<string> begin_ani_data;
/*
   _      __
  (_)__  / /____    _______  ___  __ _
 / / _ \/ __/ _ \  / __/ _ \/ _ \/  ' \
/_/\___/\__/\___/ /_/  \___/\___/_/_/_/
*/
	vector<string> into_room_ani_data;
	/*
            __
 ___  __ __/ /_  _______  ___  __ _
/ _ \/ // / __/ / __/ _ \/ _ \/  ' \
\___/\_,_/\__/ /_/  \___/\___/_/_/_/
	*/
	vector<string> quilt_room_ani_data;
	/*
            _ ____ 
 ___ ___ __(_) / /_
/ _ `/ // / / / __/
\_, /\_,_/_/_/\__/ 
 /_/
	*/
	vector<string> quilt_ani_data;
};

#endif