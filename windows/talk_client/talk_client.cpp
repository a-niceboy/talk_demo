#include "talk_client.h"
#define	ACCOUNT_SIGN		"account##"
#define	ROOM_ID_SIGN		"room_id##"
#define	CREATE_ROOM_SIGN	"create_room##"
#define	JOIN_ROOM_SIGN		"join_room##"
#define	TALK_DATA_SIGN		"talk_data##"
/*
==================================================
<<<<<<<<<<<<<<<<<   talk demo   >>>>>>>>>>>>>>>>>
==================================================
||												||
||												||
||												||
||												||
||												||
||												||
||												||
||												||
||												||
||												||
==================================================
*/


inline void draw_weight(const int& weight_length)
{
	for (int i = 0; i < weight_length; i++)
	{
		cout << "=";
	}
	cout << endl;
}

inline void draw_title(const int& weight_length)
{
	int icon_length = (weight_length - 15) / 2;
	for (int i = 0; i < icon_length; i++)
	{
		cout << "<";
	}
	cout << "   talk demo   ";
	for (int i = 0; i < icon_length; i++)
	{
		cout << ">";
	}
	cout << endl;
}

TalkClient::TalkClient(const string& host, const int& post)
	: m_stop(false)
	, m_server_sockfd(0)
	, m_host(host)
	, m_post(post)
	, m_room_id(0)
	, is_full(false)
{	

}

TalkClient::~TalkClient() 
{
	cout << "quilt ..." << endl;
	//关闭连接
	m_stop = true;
	if (m_thrd.joinable())
	{
		m_thrd.join();
	}
	closesocket(m_server_sockfd);

	WSACleanup();
	cout << "quilt ok" << endl;
}

void TalkClient::init()
{
	init_ani_data();
	init_window();
	init_socket();
}

void TalkClient::init_ani_data()
{
	string end_s = "\n";
	string begin_s = "             __                     __       ____";
	begin_ani_data.push_back(begin_s);
	begin_s = " _    _____ / /______  __ _  ___   / /____ _/ / /__";
	begin_ani_data.push_back(begin_s);
	begin_s = "| |/|/ / -_) / __/ _ \\/  ' \\/ -_) / __/ _ `/ /  '_/";
	begin_ani_data.push_back(begin_s);
	begin_s = "|__,__/\\__/_/\\__/\\___/_/_/_/\\__/  \\__/\\_,_/_/_/\\_\\";
	begin_ani_data.push_back(begin_s);
	begin_s = end_s;
	begin_ani_data.push_back(begin_s);

	string into_room = "   _      __";
	into_room_ani_data.push_back(into_room);
	into_room = "  (_)__  / /____    _______  ___  __ _";
	into_room_ani_data.push_back(into_room);
	into_room = " / / _ \\/ __/ _ \\  / __/ _ \\/ _ \\/  ' \\";
	into_room_ani_data.push_back(into_room);
	into_room = "/_/\\___/\\__/\\___/ /_/  \\___/\\___/_/_/_/";
	into_room_ani_data.push_back(into_room);
	into_room = end_s;
	into_room_ani_data.push_back(into_room);

	string quilt_room = "            __";
	quilt_room_ani_data.push_back(quilt_room);
	quilt_room = " ___  __ __/ /_  _______  ___  __ _";
	quilt_room_ani_data.push_back(quilt_room);
	quilt_room = "/ _ \\/ // / __/ / __/ _ \\/ _ \\/  ' \\";
	quilt_room_ani_data.push_back(quilt_room);
	quilt_room = "\\___/\\_,_/\\__/ /_/  \\___/\\___/_/_/_/";
	quilt_room_ani_data.push_back(quilt_room);
	quilt_room = end_s;
	quilt_room_ani_data.push_back(quilt_room);

	string quilt = "            _ ____";
	quilt_ani_data.push_back(quilt);
	quilt = " ___ ___ __(_) / /_";
	quilt_ani_data.push_back(quilt);
	quilt = "/ _ `/ // / / / __/";
	quilt_ani_data.push_back(quilt);
	quilt = "\\_, /\\_,_/_/_/\\__/";
	quilt_ani_data.push_back(quilt);
	quilt = " /_/";
	quilt_ani_data.push_back(quilt);
}

void TalkClient::init_window()
{
	string height_s = to_string(HEIGHT);
	string weight_s = to_string(WEIGHT);
	string ctl_win = "mode con cols=" + weight_s + " lines=" + height_s;
	system(ctl_win.c_str());

	//固定窗口
	HWND hWnd = GetConsoleWindow(); //获得cmd窗口句柄
	RECT rc;
	GetWindowRect(hWnd, &rc); //获得cmd窗口对应矩形

	//改变cmd窗口风格
	SetWindowLongPtr(hWnd,
		GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX);
	//因为风格涉及到边框改变，必须调用SetWindowPos，否则无效果
	SetWindowPos(hWnd,
		NULL,
		rc.left,
		rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		NULL);
}

void TalkClient::init_socket()
{
	cout << "connect server ... " << endl;
	//初始化网络环境	
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cerr << "WSAStartup failed" << endl;
		m_stop = true;
		return;
	}

	// 初始化完成，创建一个TCP的socket	
	m_server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_server_sockfd == INVALID_SOCKET)
	{
		cerr << "socket failed" << endl;
		m_stop = true;
		WSACleanup();
		return;
	}
	//指定连接的服务端信息
	SOCKADDR_IN addrServ;
	addrServ.sin_family = AF_INET;
	addrServ.sin_port = htons(m_post);
	inet_pton(AF_INET, m_host.c_str(), &addrServ.sin_addr);

	//开始连接
	int ret = connect(m_server_sockfd, (SOCKADDR*)&addrServ, sizeof(SOCKADDR));

	if (SOCKET_ERROR == ret)
	{
		cerr << "socket connect failed" << endl;
		m_stop = true;
		WSACleanup();
		closesocket(m_server_sockfd);
		return;
	}

	cout << "connect server success " << endl;	
}

void TalkClient::run()
{
	Sleep(1000);
	if (!m_stop)
	{
		begin_ani();
		menu();
		input_nick_name();

		into_room_ani();
		talking();
	}
}

void TalkClient::begin_ani()
{
	size_t max_len;
	max_len = begin_ani_data[0].size();
	for (size_t i = 1; i != begin_ani_data.size(); i++)
	{
		if (begin_ani_data[i].size() > max_len)
		{
			max_len = begin_ani_data[i].size();
		}
	}
	max_len += begin_ani_data.size();
	for (size_t i = 0; i != max_len; i++)
	{
		system("cls");
		for (size_t j = 0; j != begin_ani_data.size(); j++)
		{
			size_t temp = i;
			//倾斜绘制
			temp = (int)(temp - j) < 0 ? 0 : (temp - j);
			if (temp > begin_ani_data[j].size())
			{
				temp = begin_ani_data[j].size();
			}			
			cout << begin_ani_data[j].substr(0, temp) << endl;
		}
		Sleep(5);
	}

	Sleep(2000);
}

void TalkClient::input_nick_name()
{
	while (true)
	{
		string nick_name;
		int flag = 0;
		while (true)
		{
			system("cls");
			cout << "plase input your nick name for this talk:" << endl;

			nick_name.clear();
			cin.clear();
			cin >> nick_name;

			cout << "are your sure use " << nick_name << " ?" << endl;
			cout << "plase input: 1(yes), 0(no)" << endl;

			cin.clear();
			cin >> flag;
			if (flag == 1)
			{
				break;
			}
		}

		string out_data = ACCOUNT_SIGN;
		out_data += nick_name;
		int ret = send(m_server_sockfd, out_data.c_str(), out_data.size(), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "send error" << endl;
			closesocket(m_server_sockfd);
			return;
		}

		char buf[DATA_SIZE];
		memset(buf, 0, sizeof(buf));
		ret = recv(m_server_sockfd, buf, sizeof(buf), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "recv error" << endl;
			closesocket(m_server_sockfd);
			return;
		}
		string readbuf(buf, buf + strlen(buf));
		cout << readbuf << endl;
		if (readbuf == "ok")
		{
			break;
		}
	}

}

void TalkClient::menu()
{
	while (true)
	{
		system("cls");

		cout << "plase input: 0, 1, 2 ..." << endl;
		cout << endl;
		cout << "0: go to normal room" << endl;
		cout << "1: create your room" << endl;
		cout << "2: join other room" << endl;
		cout << endl;

		int flag = 0;
		cin >> flag;

		if (flag == 0)
		{
			join_room();
			break;
		}
		else if (flag == 1)
		{
			create_room();
			break;
		}
		else if (flag == 2)
		{
			join_room(true);
			break;
		}
		else
		{
			cout << "you input error" << endl;
			Sleep(1000);
		}
	}

}

void TalkClient::create_room()
{
	string out_data = CREATE_ROOM_SIGN;
	int ret = send(m_server_sockfd, out_data.c_str(), out_data.size(), 0);
	if (SOCKET_ERROR == ret)
	{
		cerr << "send error" << endl;
		closesocket(m_server_sockfd);
		return;
	}

	char buf[DATA_SIZE];
	memset(buf, 0, sizeof(buf));
	ret = recv(m_server_sockfd, buf, sizeof(buf), 0);
	if (SOCKET_ERROR == ret)
	{
		cerr << "recv error" << endl;
		closesocket(m_server_sockfd);
		return;
	}
	string readbuf(buf, buf + strlen(buf));
	size_t pos = readbuf.find(ROOM_ID_SIGN);
	string icon = ROOM_ID_SIGN;
	if (pos == string::npos)
	{
		cerr << "room_id no find" << endl;
	}
	else
	{
		m_room_id = atoi(readbuf.substr(pos + icon.size(), readbuf.size() - pos - icon.size()).c_str());
		if (m_room_id == 0)
		{
			cerr << "room_id 0" << endl;
		}
		else
		{
			cout << "create room success " << endl;
		}
	}
	Sleep(2000);
}

void TalkClient::join_room(const bool& is_private)
{
	int room_id = 0;
	while (true)
	{		
		system("cls");
		if (is_private)
		{
			cout << "plase input room id:" << endl;
			while (true)
			{
				cin >> room_id;
				if (room_id == 0)
				{
					cerr << "room id coun't 0!" << endl;
				}
				else
				{
					break;
				}
			}
		}

		string out_data = JOIN_ROOM_SIGN;
		out_data += to_string(room_id);
		int ret = send(m_server_sockfd, out_data.c_str(), out_data.size(), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "send error" << endl;
			closesocket(m_server_sockfd);
			return;
		}

		char buf[DATA_SIZE];
		memset(buf, 0, sizeof(buf));
		ret = recv(m_server_sockfd, buf, sizeof(buf), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "recv error" << endl;
			closesocket(m_server_sockfd);
			return;
		}
		string readbuf(buf, buf + strlen(buf));
		cout << readbuf << endl;
		if (readbuf == "ok")
		{
			m_room_id = room_id;
			break;
		}
		else
		{
			Sleep(1000);
		}
	}
	Sleep(2000);
}

void TalkClient::into_room_ani()
{
	size_t max_len;
	max_len = into_room_ani_data[0].size();
	for (size_t i = 1; i != into_room_ani_data.size(); i++)
	{
		if (into_room_ani_data[i].size() > max_len)
		{
			max_len = into_room_ani_data[i].size();
		}
	}
	max_len += into_room_ani_data.size();
	for (size_t i = 0; i != max_len; i++)
	{
		system("cls");
		for (size_t j = 0; j != into_room_ani_data.size(); j++)
		{
			size_t temp = i;
			//倾斜绘制
			temp = (int)(temp - j) < 0 ? 0 : (temp - j);
			if (temp > into_room_ani_data[j].size())
			{
				temp = into_room_ani_data[j].size();
			}
			cout << into_room_ani_data[j].substr(0, temp) << endl;
		}
		Sleep(5);
	}

	Sleep(2000);
}

void TalkClient::talking()
{
	m_thrd = std::thread(&TalkClient::recv_data, this);
	draw_windows();
	send_data();
}

void TalkClient::send_data_size(const string& dada)
{
	int size = dada.size();
	cout << "size: " << size << endl;
	size = htons(size);

	int ret = send(m_server_sockfd, (char*)size, sizeof(size), 0);
	if (SOCKET_ERROR == ret)
	{
		cerr << "send error" << endl;
		closesocket(m_server_sockfd);
		return;
	}
}

void TalkClient::send_data()
{
	int ret = 0;
	while (!m_stop)
	{
		cin.clear();
		string out_body;
		cin >> out_body;
		//send_data_size(in_buf);

		int size = out_body.size();
		//cout << "size: " << size << endl;
		size = htons(size);

		string out_data = TALK_DATA_SIGN;
		out_data += out_body;
		//int ret = send(m_server_sockfd, (char*)size, sizeof(size), 0);
		int ret = send(m_server_sockfd, out_data.c_str(), out_data.size(), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "send error" << endl;
			closesocket(m_server_sockfd);
			return;
		}
	}
}

void TalkClient::recv_data()
{
	int ret = 0;
	while (!m_stop)
	{
		char buf[DATA_SIZE];
		memset(buf, 0, sizeof(buf));
		ret = recv(m_server_sockfd, buf, sizeof(buf), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "recv error" << endl;
			closesocket(m_server_sockfd);
			return;
		}
		string readbuf(buf, buf + strlen(buf));
		m_read_buf.clear();
		m_read_buf = readbuf;
		draw_windows();//cout << readbuf << endl;
	}
}

inline void TalkClient::draw_windows()
{
	system("cls");
	cout << "room id: " << m_room_id << endl;
	draw_weight(WEIGHT);
	draw_title(WEIGHT);
	draw_weight(WEIGHT);

	do
	{
		if (m_read_buf.empty())
			break;
		if (!is_full)
		{
			int temp_size = talk_datas.size();
			for (size_t i = 0; i != temp_size; i++)
			{
				if (talk_datas.back() == "")
				{
					talk_datas.pop_back();
				}
				else
				{
					break;
				}
			}
		}
		size_t pos = m_read_buf.find("\n");

		string person = m_read_buf.substr(0, pos);
		talk_datas.push_back(person);

		string body = m_read_buf.substr(pos + 1, m_read_buf.size() - pos);
		if (body.size() > WEIGHT)
		{
			for (int i = 0; i != (body.size() / WEIGHT + 1); i++)
			{
				string temp;
				if (i == (body.size() / WEIGHT + 1))
				{
					temp = body.substr(i * WEIGHT, body.size() - i * WEIGHT);
				}
				else
				{
					temp = body.substr(i * WEIGHT, WEIGHT);
				}
				talk_datas.push_back(temp);
			}
		}
		else
		{
			talk_datas.push_back(body);
		}

	} while (0);

	if (talk_datas.size() < BODY_HEIGHT)
	{
		int temp_size = talk_datas.size();
		for (int i = 0; i != BODY_HEIGHT - temp_size; i++)
		{
			talk_datas.push_back("");
		}
	}
	else
	{
		int temp_size = talk_datas.size();
		for (int i = 0; i < (temp_size - BODY_HEIGHT); i++)
		{
			talk_datas.pop_front();
		}
	}

	if (talk_datas.back() != "")
	{
		is_full = true;
	}

	deque<string> temp = talk_datas;

	for (int i = 0; i != talk_datas.size(); i++)
	{
		cout << temp.front() << endl;
		temp.pop_front();
	}

	draw_weight(WEIGHT);
}

void TalkClient::quilt_room()
{

}

void TalkClient::quilt_room_ani()
{

}

void TalkClient::quilt()
{

}

void TalkClient::quilt_ani()
{

}
