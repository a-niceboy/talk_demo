#include "talk_server.h"

TalkServer::TalkServer()
	: m_init(false)
	, m_stop(false)
	, m_server_sockfd(-1)
	, m_epoll_sockfd(-1)
{

}

TalkServer::~TalkServer()
{
	if (m_thrd.joinable())
	{
		m_thrd.join();
	}
}

void TalkServer::init()
{
	cout << "=======  init  =======" << endl;
	//创建服务器fd	
	m_server_sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	// 设置 地址复用
	socklen_t val = 1;
	Setsockopt(m_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	//设置非阻塞-------放在绑定之前
	int iMode = 1;
	Ioctl(m_server_sockfd, FIONBIO, &iMode);

	//绑定
	Bind(m_server_sockfd, AF_INET, POST, htonl(INADDR_ANY));

	//设置监听数
	Listen(m_server_sockfd, 128);

	//创建epoll
	m_epoll_sockfd = Epoll_create(500);

	//事件信息 event.events = EPOLLIN | EPOLLET | EPOLLPRI | EPOLLOUT | EPOLLRDHUP;
	epoll_event e_event;
	e_event.data.fd = m_server_sockfd;
	e_event.events = EPOLLIN | EPOLLET;

	//事件添加
	Epoll_ctl(m_epoll_sockfd, EPOLL_CTL_ADD, m_server_sockfd, &e_event);

	m_init = true;
	cout << "=======  init end  =======" << endl;
}

void TalkServer::run()
{
	if (!m_init)
	{
		init();
	}

	run_epoll();
}

void TalkServer::run_epoll() 
{
	while (!m_stop)
	{
		cout << "====================" << endl;
		epoll_event all_events[500] = { 0 };
		int have_sockfd = epoll_wait(m_epoll_sockfd, all_events, 500, -1);
		cout << "have sockfds: " << have_sockfd << endl;

		if (have_sockfd == 0)
		{
			continue;
		}
		else if (have_sockfd < 0)
		{
			cerr << "epoll_wait error." << endl;
			m_stop = true;
			break;
		}

		for (int i = 0; i < have_sockfd; i++)
		{
			int counter_fd = all_events[i].data.fd;
			if (counter_fd == m_server_sockfd)
			{
				run_accept();
			}						
			//非服务器可读
			else if (all_events[i].events & EPOLLIN)
			{
				string readbuf;
				while (true)
				{
					char buf[DATA_SIZE];
					memset(buf, 0, sizeof(buf));
					int ret = read(counter_fd, buf, sizeof(buf));
					//对端关闭
					if (ret == 0)
					{
						//事件删除
						Epoll_ctl(m_epoll_sockfd, EPOLL_CTL_DEL, counter_fd, &all_events[i]);
						cout << "epoll del " << endl;

						map<int, User>::iterator iter = m_users.find(counter_fd);
						if (iter != m_users.end())
						{
							m_users.erase(iter);
						}

						break;
					}
					//用于非阻塞read
					else if (ret < 0)
					{
						//表示中断，处理
						if (errno == EINTR)
						{
							cout << "\nread EINTR" << endl;
							continue;
						}
						//没有数据先结束稍后再说
						if (errno == EAGAIN)
						{
							cout << "waiting..." << endl;
							break;
						}
					}

					string temp(buf, buf + strlen(buf));
					readbuf += temp;
				}

				cout << "readbuf size: " << readbuf.size() << endl;
				char * p = &readbuf[0];
				int size = ntohl(*(int*)p);
				cout << "size: " << size << endl;
				//readbuf = readbuf.substr(3, size);
				cout << "client fd: " << counter_fd << "\n" << "send: " << readbuf << endl;

				//data_center(counter_fd, readbuf);
			}
			else if (all_events[i].events & EPOLLOUT)
			{
				cout << "## EPOLLOUT ##" << endl;
			}	
		}
	}
	close(m_server_sockfd);
	return;
}

void TalkServer::run_accept()
{
	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr));
	socklen_t client_addr_len = sizeof(sockaddr_in);

	int client_fd = Accept(m_server_sockfd, (struct sockaddr*) & client_addr, &client_addr_len);

	cout << "new client connect" << endl;
	char client_ip[128];
	cout << "client ip: \n"
		<< inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip))
		<< endl;

	//设置非阻塞        
	int iMode = 1;
	Ioctl(client_fd, FIONBIO, &iMode);

	epoll_event event;
	event.data.fd = client_fd;
	event.events = EPOLLIN | EPOLLET;
	//事件绑定
	Epoll_ctl(m_epoll_sockfd, EPOLL_CTL_ADD, client_fd, &event);

	User u;
	m_users[client_fd] = u;
}

void TalkServer::data_center(const int& counter_fd, const string& readbuf)
{
	cout << "user size:" << m_users.size() << endl;
	for (map<int, User>::iterator iter = m_users.begin(); iter != m_users.end(); iter++)
	{
		string out_data;
		if (iter->first == counter_fd)
		{
			out_data = "you say:\n";
		}
		else
		{
			out_data = "other say:\n";
		}
		out_data += readbuf;
		write(iter->first, out_data.c_str(), out_data.size());
	}
}