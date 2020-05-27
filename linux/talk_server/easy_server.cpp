#include <stdio.h>
#include <string.h>		//bzero

#include <sys/socket.h>	//socket bind listen accept
#include <arpa/inet.h>	//hton
#include <unistd.h>		//read write sleep

#include <sys/ioctl.h>	//ioctl 设置非阻塞
#include <sys/epoll.h>

#include <errno.h>

#include <vector>
#include <iostream>

using namespace std;

vector<int> users;

void run_server()
{
	cout << "run_server ..." << endl;

	//创建服务器fd	
	int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sockfd == -1)
	{
		cout << "server_sockfd make error." << endl;
		return ;
	}

	//绑定信息
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(12345);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 设置 地址复用
	socklen_t val = 1;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
	{
		cout << "TCPServer  error : " << endl;
		close(server_sockfd);
		return ;
	}

	//设置非阻塞-------放在绑定之前
	int iMode = 1;
	if (ioctl(server_sockfd, FIONBIO, &iMode))
	{
		cout << "ioctl to no block error!\n" << endl;
		return ;
	}

	//绑定
	if (bind(server_sockfd, (struct sockaddr*) & server_addr, sizeof(server_addr)))
	{
		cout << "\n" << "bind error." << endl;
		close(server_sockfd);
		return ;
	}

	//设置监听数
	if (listen(server_sockfd, 128))
	{
		cout << "\n" << "listen error." << endl;
		close(server_sockfd);
		return ;
	}

	//创建epoll
	int epoll_sockfd = epoll_create(999);
	if (epoll_sockfd < 0)
	{
		close(server_sockfd);
		cout << "epollcreate  error : " << endl;
		return ;
	}

	//事件信息
	epoll_event event;
	event.data.fd = server_sockfd;
	//event.events = EPOLLIN | EPOLLET | EPOLLPRI | EPOLLOUT;
	event.events = EPOLLIN | EPOLLET | EPOLLPRI | EPOLLOUT;//| EPOLLRDHUP

	//事件添加
	if (epoll_ctl(epoll_sockfd, EPOLL_CTL_ADD, server_sockfd, &event) < 0)
	{
		cout << "epoll add events server error " << endl;
		close(epoll_sockfd);
		close(server_sockfd);
		return ;
	}

	while (true)
	{
		cout << "server waiting..." << endl;
		epoll_event events[999] = { 0 };
		int have_sockfd = epoll_wait(epoll_sockfd, events, 999, -1);
		cout << "have_sockfd: " << have_sockfd << endl;
		if (have_sockfd == 0)
		{
			continue;
		}
		else if (have_sockfd < 0)
		{
			cout << "epoll_wait error" << endl;
			return ;
		}

		for (int i = 0; i < have_sockfd; i++)
		{
			int listen_sockfd = events[i].data.fd;
			if (listen_sockfd == server_sockfd)
			{
				cout << "new client connect..." << endl;
				sockaddr_in client_addr;
				socklen_t client_addr_len = sizeof(sockaddr_in);

				while (true)
				{
					int client_fd = accept(server_sockfd, (struct sockaddr*) & client_addr, &client_addr_len);
					if (client_fd < 0)
					{
						cout << "accept error." << endl;
						break;
					}

					char client_ip[128];
					cout << "client_ip: "
						<< inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip))
						<< endl;

					//设置非阻塞        
					int iMode = 1;
					if (ioctl(client_fd, FIONBIO, &iMode))
					{
						cout << "ioctl to no block error!\n" << endl;
						continue;
					}

					event.data.fd = client_fd;
					event.events = EPOLLIN | EPOLLET | EPOLLPRI | EPOLLOUT;
					//事件绑定
					if (epoll_ctl(epoll_sockfd, EPOLL_CTL_ADD, client_fd, &event) < 0)
					{
						cout << "epoll add client fd: " << client_fd << " error" << endl;
						close(client_fd);
						break;
					}
					cout << "epoll add fd: " << client_fd << endl;
					users.push_back(client_fd);
				}
			}
			else if (events[i].events & EPOLLIN)
			{
				cout << "client: " << listen_sockfd << endl;
				vector <uint8_t> readbuf;
				bool isClose = false;
				while (true)
				{
					//_bufSize  87380
					char buf[87380];
					int bufSize = read(listen_sockfd, buf, sizeof(buf));
					//对端关闭
					if (bufSize == 0)
					{
						//事件删除
						if (epoll_ctl(epoll_sockfd, EPOLL_CTL_DEL, listen_sockfd, &events[i]) < 0)
						{
							cout << "epoll del client fd: " << listen_sockfd << " error" << endl;
							close(listen_sockfd);
							break;
						}
						cout << "epoll del client fd: " << listen_sockfd  << endl;

						for (vector<int>::iterator iter = users.begin(); iter != users.end(); iter++)
						{
							if (*iter == listen_sockfd)
							{
								users.erase(iter);
								break;
							}
						}
						

						isClose = true;
						break;
					}
					//用于非阻塞read
					else if (bufSize < 0)
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
						isClose = true;
					}
					copy(buf, buf + bufSize, back_inserter(readbuf));
				}
				string this_recv(readbuf.begin(), readbuf.end());
				cout << "client fd: " << listen_sockfd << "\n" << "send: " << this_recv << endl;
				//TcpRecvTask* recvTask = new TcpRecvTask(fd, std::move(readbuf), isClose);
				//GameManager::getInstance()->getThreadPool()->addTask(recvTask);

				for (size_t i = 0; i != users.size(); i++)
				{
					string out_data;
					if (users[i] == listen_sockfd)
					{
						out_data = "you say:\n";
					}
					else
					{
						out_data = "other say:\n";
					}
					out_data += this_recv;
					write(users[i], out_data.c_str(), out_data.size());
				}
			}
			else if (events[i].events & EPOLLPRI)
			{
				cout << "------------   EPOLLPRI   ---------------" << endl;
			}
			else if (events[i].events & EPOLLOUT)
			{
				cout << "------------   EPOLLOUT   ---------------" << endl;
			}
		}
	}
	close(server_sockfd);
	return ;
}

int main() 
{
	run_server();
	return 0;
}