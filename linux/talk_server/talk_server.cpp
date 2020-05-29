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
	//����������fd	
	m_server_sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	// ���� ��ַ����
	socklen_t val = 1;
	Setsockopt(m_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	//���÷�����-------���ڰ�֮ǰ
	int iMode = 1;
	Ioctl(m_server_sockfd, FIONBIO, &iMode);

	//��
	Bind(m_server_sockfd, AF_INET, POST, htonl(INADDR_ANY));

	//���ü�����
	Listen(m_server_sockfd, 128);

	//����epoll
	m_epoll_sockfd = Epoll_create(500);

	//�¼���Ϣ event.events = EPOLLIN | EPOLLET | EPOLLPRI | EPOLLOUT | EPOLLRDHUP;
	epoll_event e_event;
	e_event.data.fd = m_server_sockfd;
	e_event.events = EPOLLIN | EPOLLET;

	//�¼����
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
			//�Ƿ������ɶ�
			else if (all_events[i].events & EPOLLIN)
			{
				string readbuf;
				while (true)
				{
					char buf[DATA_SIZE];
					memset(buf, 0, sizeof(buf));
					int ret = read(counter_fd, buf, sizeof(buf));
					//�Զ˹ر�
					if (ret == 0)
					{
						//�¼�ɾ��
						Epoll_ctl(m_epoll_sockfd, EPOLL_CTL_DEL, counter_fd, &all_events[i]);
						cout << "epoll del " << endl;

						map<int, User>::iterator iter = m_users.find(counter_fd);
						if (iter != m_users.end())
						{
							m_users.erase(iter);
						}

						break;
					}
					//���ڷ�����read
					else if (ret < 0)
					{
						//��ʾ�жϣ�����
						if (errno == EINTR)
						{
							cout << "\nread EINTR" << endl;
							continue;
						}
						//û�������Ƚ����Ժ���˵
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

	//���÷�����        
	int iMode = 1;
	Ioctl(client_fd, FIONBIO, &iMode);

	epoll_event event;
	event.data.fd = client_fd;
	event.events = EPOLLIN | EPOLLET;
	//�¼���
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