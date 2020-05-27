#include "talk_server.h"

TalkServer::TalkServer()
	: m_stop(false)
{
	cout << "=======  init  =======" << endl;
	//创建服务器fd	
	m_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_server_sockfd == -1)
	{
		cerr << "m_server_sockfd make error." << endl;
		m_stop = true;
		return;
	}

	// 设置 地址复用
	socklen_t val = 1;
	if (setsockopt(m_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
	{
		cerr << "setsockopt error." << endl;
		close(m_server_sockfd);
		m_stop = true;
		return;
	}

	//设置非阻塞-------放在绑定之前
	int iMode = 1;
	if (ioctl(m_server_sockfd, FIONBIO, &iMode))
	{
		cerr << "ioctl to no block error." << endl;
		m_stop = true;
		return;
	}

	//绑定信息
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(HOST);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定
	if (bind(m_server_sockfd, (struct sockaddr*) & server_addr, sizeof(server_addr)))
	{
		cerr << "bind error." << endl;
		close(m_server_sockfd);
		m_stop = true;
		return;
	}

	//设置监听数
	if (listen(m_server_sockfd, 128))
	{
		cerr << "listen error." << endl;
		close(m_server_sockfd);
		m_stop = true;
		return;
	}
	//创建epoll
	m_epoll_sockfd = epoll_create(1);
	if (m_epoll_sockfd < 0)
	{
		close(m_server_sockfd);
		cerr << "epoll create error." << endl;
		m_stop = true;
		return;
	}
	cout << "server sockfd: " << m_server_sockfd << endl;

	//事件信息
	epoll_event e_event;
	e_event.data.fd = m_server_sockfd;
	//event.events = EPOLLIN | EPOLLET | EPOLLPRI | EPOLLOUT | EPOLLRDHUP;
	e_event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP;

	cout << "EPOLLIN: " << EPOLLIN << endl;
	cout << "EPOLLET: " << EPOLLET << endl;
	cout << "EPOLLPRI: " << EPOLLPRI << endl;
	cout << "EPOLLOUT: " << EPOLLOUT << endl;
	cout << "EPOLLRDHUP: " << EPOLLRDHUP << endl;

	//事件添加
	if (epoll_ctl(m_epoll_sockfd, EPOLL_CTL_ADD, m_server_sockfd, &e_event) < 0)
	{
		cerr << "epoll add events server error " << endl;
		m_stop = true;
		close(m_epoll_sockfd);
		close(m_server_sockfd);
		return;
	}
	cout << "=======  init end  =======" << endl;
}

TalkServer::~TalkServer()
{
	if (m_thrd.joinable())
	{
		m_thrd.join();
	}
}


void TalkServer::run_server()
{
	run_epoll();
}

void TalkServer::run_epoll() 
{
	while (!m_stop)
	{
		cout << "==== server waiting" << endl;
		epoll_event all_events[10] = { 0 };
		int have_sockfd = epoll_wait(m_epoll_sockfd, all_events, 10, -1);
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
			m_listen_sockfd = all_events[i].data.fd;
			if (m_listen_sockfd == m_server_sockfd)
			{
				cout << "m_server_sockfd events: " << all_events[i].events << endl;
				run_accept();
			}
						
			//只有客户端会主动发信息
			else if (all_events[i].events & EPOLLIN)
			{
				cout << "client: " << m_listen_sockfd << endl;				
				string readbuf;
				while (true)
				{
					char buf[DATA_SIZE];
					memset(buf, 0, sizeof(buf));
					int ret = read(m_listen_sockfd, buf, sizeof(buf));
					//对端关闭
					if (ret == 0)
					{
						//事件删除
						if (epoll_ctl(m_epoll_sockfd, EPOLL_CTL_DEL, m_listen_sockfd, &all_events[i]) < 0)
						{
							cerr << "epoll del client fd: " << m_listen_sockfd << " error" << endl;
							close(m_listen_sockfd);
							break;
						}
						cout << "epoll del client fd: " << m_listen_sockfd << endl;

						map<int, User>::iterator iter = m_users.find(m_listen_sockfd);
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
				cout << "client fd: " << m_listen_sockfd << "\n" << "send: " << readbuf << endl;

				//data_center(readbuf);
			}
			else if (all_events[i].events & EPOLLPRI)
			{
				cout << "## EPOLLPRI ##" << endl;
			}
			else if (all_events[i].events & EPOLLOUT)
			{
				cout << "## EPOLLOUT ##" << endl;
			}
			else if (all_events[i].events & EPOLLRDHUP)
			{
				cout << "## EPOLLRDHUP ##" << endl;
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

	int client_fd = accept(m_server_sockfd, (struct sockaddr*) & client_addr, &client_addr_len);
	if (client_fd < 0)
	{
		cerr << "accept error." << endl;
		m_stop = true;
		return;
	}
	cout << "new client connect" << endl;
	char client_ip[128];
	cout << "client ip: \n"
		<< inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip))
		<< endl;

	//设置非阻塞        
	int iMode = 1;
	if (ioctl(client_fd, FIONBIO, &iMode))
	{
		cerr << "ioctl to no block error!\n" << endl;
		m_stop = true;
		return;
	}

	epoll_event event;
	event.data.fd = client_fd;
	event.events = EPOLLIN | EPOLLET | EPOLLOUT;
	//事件绑定
	if (epoll_ctl(m_epoll_sockfd, EPOLL_CTL_ADD, client_fd, &event) < 0)
	{
		cerr << "epoll add client fd: " << client_fd << " error" << endl;
		close(client_fd);
		m_stop = true;
		return;
	}
	User u;
	m_users[client_fd] = u;
}

void TalkServer::data_center(const string& readbuf)
{
	cout << "user size:" << m_users.size() << endl;
	for (map<int, User>::iterator iter = m_users.begin(); iter != m_users.end(); iter++)
	{
		string out_data;
		if (iter->first == m_listen_sockfd)
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