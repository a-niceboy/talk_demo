#include "talk_client.h"

TalkClient::TalkClient(const string& host, const int& post)
{	
	//初始化网络环境	
	WSADATA wsa;	
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{	
		cerr << "WSAStartup failed" << endl;
		return ;	
	}

	// 初始化完成，创建一个TCP的socket	
	m_server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(m_server_sockfd == INVALID_SOCKET)
	{	
		cerr << "socket failed" << endl;
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
		WSACleanup();	
		closesocket(m_server_sockfd);
		return ;
	}	
	m_thrd = std::thread(&TalkClient::recv_data, this);
	send_data();
}

TalkClient::~TalkClient() 
{
	//关闭连接
	closesocket(m_server_sockfd);
	WSACleanup();
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
	while (true)
	{
		cin.clear();
		string in_buf;
		cin >> in_buf;
		//send_data_size(in_buf);

		int size = in_buf.size();
		cout << "size: " << size << endl;
		size = htons(size);

		int ret = send(m_server_sockfd, (char*)size, sizeof(size), 0);
		if (SOCKET_ERROR == ret)
		{
			cerr << "send error" << endl;
			closesocket(m_server_sockfd);
			return;
		}
		//ret = send(m_server_sockfd, in_buf.c_str(), in_buf.size(), 0);
		//if (SOCKET_ERROR == ret)
		//{
		//	cerr << "send error" << endl;
		//	closesocket(m_server_sockfd);
		//	return;
		//}
	}
}

void TalkClient::recv_data()
{
	int ret = 0;
	while (true)
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
		cout << readbuf << endl;
	}
}
