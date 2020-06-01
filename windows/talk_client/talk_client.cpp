#include "talk_client.h"
/*
==================================================
<<<<<<<<<<<<<<<<<   talk demo   >>>>>>>>>>>>>>>>>
==================================================
|												||
|												||
|												||
|												||
|												||
|												||
|												||
|												||
|												||
|												||
==================================================


*/

TalkClient::TalkClient(const string& host, const int& post)
	: m_stop(false)
	, m_server_sockfd(0)
{	
	//初始化网络环境	
	WSADATA wsa;	
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{	
		cerr << "WSAStartup failed" << endl;
		m_stop = true;
		return ;	
	}

	// 初始化完成，创建一个TCP的socket	
	m_server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(m_server_sockfd == INVALID_SOCKET)
	{	
		cerr << "socket failed" << endl;
		m_stop = true;
		WSACleanup();
		return ;	
	}
	//指定连接的服务端信息
	SOCKADDR_IN addrServ;
	addrServ.sin_family = AF_INET;
	addrServ.sin_port = htons(post);
	inet_pton(AF_INET, host.c_str(), &addrServ.sin_addr);

	//开始连接
	int ret = connect(m_server_sockfd,(SOCKADDR*)&addrServ,sizeof(SOCKADDR));

	if (SOCKET_ERROR == ret)	
	{		
		cerr << "socket connect failed" << endl;
		m_stop = true;
		WSACleanup();	
		closesocket(m_server_sockfd);
		return ;
	}	

	cout << "connect server success " << endl;
	m_thrd = std::thread(&TalkClient::recv_data, this);
	
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


void TalkClient::run()
{
	if (!m_stop)
	{
		draw_windows();
		send_data();
	}
}

void TalkClient::send_data_size(const string& dada)
{
	int size = dada.size();
	cout << "size: " << size  << endl;
	size = htons(size);

	int ret = send(m_server_sockfd, (char *)size, sizeof(size), 0);
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
		string in_buf;
		cin >> in_buf;
		//send_data_size(in_buf);

		int size = in_buf.size();
		//cout << "size: " << size << endl;
		size = htons(size);

		//int ret = send(m_server_sockfd, (char*)size, sizeof(size), 0);
		int ret = send(m_server_sockfd, in_buf.c_str(), in_buf.size(), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "send error" << endl;
			closesocket(m_server_sockfd);
			return;
		}
	}
}
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

inline void TalkClient::draw_windows()
{
	system("cls");
	cout << "draw_windows " << index++ << endl;
	draw_weight(WEIGHT);
	draw_title(WEIGHT);
	draw_weight(WEIGHT);

	do
	{
		if (m_read_buf.empty())
			break;
		size_t pos = m_read_buf.find("\n");

		string person = m_read_buf.substr(0, pos);
		talk_datas.push(person);
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
				talk_datas.push(temp);
			}
		}
		else
		{
			talk_datas.push(body);
		}

	} while (0);
	
	if (talk_datas.size() < BODY_HEIGHT)
	{
		int temp_size = talk_datas.size();
		for (int i = 0; i != BODY_HEIGHT - temp_size; i++)
		{
			talk_datas.push("1");
		}
	}
	else
	{
		int temp_size = talk_datas.size();
		for (int i = 0; i < (temp_size - BODY_HEIGHT); i++)
		{
			talk_datas.pop();
		}
	}
	 
	queue<string> temp = talk_datas;

	for (int i = 0; i != talk_datas.size(); i++)
	{
		cout << temp.front() << endl;
		temp.pop();
	}

	draw_weight(WEIGHT);
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
