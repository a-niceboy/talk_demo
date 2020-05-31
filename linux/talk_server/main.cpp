#include <sys/socket.h>	//socket bind listen accept
#include <arpa/inet.h>	//hton
#include <unistd.h>		//read write sleep

#include <sys/ioctl.h>	//ioctl ���÷�����
#include <sys/epoll.h>	//epoll

#include <fcntl.h>

#include <string.h>		//bzero
#include <errno.h>
#include <map>
#include <iostream>

using namespace std;
#define	MAX_EVENTS		1024
#define	BUFLEN			4096
#define	SERV_POST		6565

map<int, string> User;

/*���������ļ������������Ϣ*/

struct myevent_s
{
	int fd = 0;											//Ҫ�������ļ�������
	int events = 0;										//��Ӧ�ļ����¼�
	void* arg;											//���Ͳ���
	void (*call_back)(void* arg);						//�ص�����

	int status = 0;										//�Ƿ��ڼ�����1->�ں�����ϣ��������� 0->���ڣ���������
	char buf[BUFLEN];									//����
	int len = 0;
	long long last_active_time = 0;						//����Ծʱ��
};

int g_efd;

struct myevent_s g_events[MAX_EVENTS + 1];

/*���ṹ�� myevect_s��Ա������ʼ��*/
void eventset(struct myevent_s* ev, int fd, void (*call_back)(void*), void* arg)
{
	ev->fd = fd;
	ev->events = 0;
	ev->arg = arg;
	ev->call_back = call_back;

	ev->status = 0;
	bzero(ev->buf, sizeof(ev->buf));
	ev->len = 0;
	//��������ò���
	ev->last_active_time = time(NULL);
}

/*�� epoll�����ĺ���� ���һ�� �ļ�������*/
void eventadd(int efd, int events, struct myevent_s* ev)
{
	int op;
	if (ev->status == 1)
	{
		op = EPOLL_CTL_MOD;
	}
	else
	{
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	struct epoll_event epv = { 0, {0} };

	epv.data.ptr = ev;
	epv.events = ev->events = events;

	if (epoll_ctl(efd, op, ev->fd, &epv) < 0)
	{
		cout << "epoll_ctl error" << endl;
		//error
	}
}

/*�� epoll�����ĺ���� ɾ��һ�� �ļ�������*/
void eventdel(int efd, struct myevent_s* ev)
{
	if (ev->status != 1)
	{
		cout << "return del" << endl;
		return;
	}

	struct epoll_event epv = { 0, {0} };

	epv.data.ptr = ev;
	ev->status = 0;

	if (epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv) < 0)
	{
		cout << "EPOLL_CTL_DEL error" << endl;
		//error
	}
}

void senddata(void* arg);

//�ͻ��˶��ص�����
void recvdata(void* arg)
{
	cout << "==== recv ..." << endl;
	struct myevent_s* ev = (struct myevent_s*)arg;

	while (true)
	{
		int len = recv(ev->fd, ev->buf, sizeof(ev->buf), 0);

		if (len > 0)
		{
			ev->len += len;
		}
		else if (len == 0)
		{
			cout << "==== client fd: " << ev->fd << " close" << endl;

			auto iter = User.find(ev->fd);
			if (iter != User.end())
			{
				User.erase(iter);
			}

			eventdel(g_efd, ev);
			close(ev->fd);
			return;
		}
		//���ڷ�����read
		else
		{
			//��ʾ�жϣ�����
			if (errno == EINTR)
			{
				cerr << "read EINTR" << endl;
				continue;
			}
			//û�������Ƚ����Ժ���˵
			if (errno == EAGAIN)
			{
				cerr << "waiting..." << endl;
				break;
			}
		}
	}
	ev->buf[ev->len] = '\0';
	eventdel(g_efd, ev);

	/*
	����
	*/
	cout << "client fd: " << ev->fd << endl;
	cout << "buf: " << ev->buf << endl;
	User[ev->fd] = ev->buf;
	eventset(ev, ev->fd, senddata, ev);
	eventadd(g_efd, (EPOLLOUT | EPOLLET), ev);

	cout << "====  recv success  ====" << endl;
}

//�ͻ���д�ص�����
void senddata(void* arg)
{
	cout << "==== send ..." << endl;
	struct myevent_s* ev = (struct myevent_s*)arg;

	string out = "you say: \n";
	//string body = 
	out += User[ev->fd];
	int flag = send(ev->fd, out.c_str(), out.size(), 0);
	cout << "client fd: " << ev->fd << endl;
	cout << "buf: " << out << endl;
	if (flag > 0)
	{
		eventdel(g_efd, ev);
		//fcntl(ev->fd, F_SETFL, O_NONBLOCK);
		eventset(ev, ev->fd, recvdata, ev);
		eventadd(g_efd, (EPOLLIN | EPOLLET), ev);

		cout << "====  send success  ====" << endl;
	}
	else
	{
		cout << "client fd: " << ev->fd << "send error" << endl;
		auto iter = User.find(ev->fd);
		if (iter != User.end())
		{
			User.erase(iter);
		}
		eventdel(g_efd, ev);
		close(ev->fd);
	}

	for (auto iter = User.begin(); iter != User.end(); )
	{
		if (iter->first == ev->fd)
		{
			iter++;
			continue;
		}

		string out = "other say: \n";
		out += User[ev->fd];
		int flag = send(iter->first, out.c_str(), out.size(), 0);
		cout << "client fd: " << iter->first << endl;
		cout << "buf: " << out << endl;
		if (flag > 0)
		{
			cout << "====  send success  ====" << endl;
			iter++;
		}
		else
		{
			iter = User.erase(iter);
			cout << "client fd: " << ev->fd << "send error" << endl;
			//close(ev->fd);
		}
	}
}

//����˻ص�����
void accept_conn(void* arg)
{
	cout << "==== have connect ..." << endl;

	struct myevent_s* ev = (struct myevent_s*)arg;
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	int i = 0;

	int client_fd = accept(ev->fd, (struct sockaddr*) & client_addr, &len);
	if (client_fd < 0)
	{
		if (errno != EAGAIN && errno != EINTR)
		{
			cerr << "client connect error" << endl;
			/*����*/
		}
		return;
	}

	do
	{
		for (i = 0; i < MAX_EVENTS; i++)
		{
			cout << "client pos: " << i << endl;
			if (g_events[i].status == 0)
			{
				break;
			}
		}
		if (i == MAX_EVENTS)
		{
			cerr << "max connect " << endl;//
			break;
		}

		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		eventset(&g_events[i], client_fd, recvdata, &g_events[i]);
		eventadd(g_efd, (EPOLLIN | EPOLLET), &g_events[i]);

	} while (0);

	cout << "====  connect success  ====" << endl;
	cout << "\n" << endl;
}

void init_listen_socket(int efd, short port)
{
	cout << "==== init ... " << endl;

	int lfd = socket(AF_INET, SOCK_STREAM, 0);

	// ���� ��ַ����
	socklen_t val = 1;
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
	{
		cerr << "setsockopt  error : " << endl;
		close(lfd);
		return;
	}
	//������
	fcntl(lfd, F_SETFL, O_NONBLOCK);

	//eventset(struct myevent_s *ev , lfd, void(*call_back)(int,int,void*), void* arg));
	eventset(&g_events[MAX_EVENTS], lfd, accept_conn, &g_events[MAX_EVENTS]);
	eventadd(efd, (EPOLLIN | EPOLLET), &g_events[MAX_EVENTS]);

	//����Ϣ
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(lfd, (sockaddr*)&addr, sizeof(addr)))
	{
		cerr << "bind error" << endl;
	}

	listen(lfd, 128);

	cout << "=======  init end  ======= " << endl;
	cout << "\n" << endl;
}

int main(int argc, char* argv[])
{
	cout << "========  server  ========" << endl;
	cout << "=====  epoll reactor  =====" << endl;

	unsigned short port = SERV_POST;
	if (argc == 2)
	{
		port = 0;// atoi(argv[1]);
	}

	g_efd = epoll_create(MAX_EVENTS + 1);
	if (g_efd < 0)
	{
		cerr << endl;
	}

	init_listen_socket(g_efd, port);

	struct epoll_event events[MAX_EVENTS + 1];

	int i = 0, check_pos = 0;
	while (true)
	{
		//��Ծ��ʱȥ��
		long long now_time = time(NULL);
		for (i = 0; i < 100; i++, check_pos++)
		{
			if (check_pos == MAX_EVENTS)
			{
				check_pos = 0;
			}

			if (g_events[check_pos].status != 1)
			{
				continue;
			}
			long long duration = now_time - g_events[check_pos].last_active_time;
			if (duration >= 10)
			{
				close(g_events[check_pos].fd);
				cout << "fd: " << g_events[check_pos].fd << " no active del" << endl;
				eventdel(g_efd, &g_events[check_pos]);
			}
		}

		int nfd = epoll_wait(g_efd, events, MAX_EVENTS + 1, -1);
		if (nfd < 0)
		{
			break;
		}

		for (i = 0; i < nfd; i++)
		{
			myevent_s* ev = (myevent_s*)events[i].data.ptr;

			if (((events[i].events & EPOLLIN) &&
				(ev->events & EPOLLIN)) ||
				((events[i].events & EPOLLOUT) &&
					(ev->events & EPOLLOUT)))
			{
				ev->call_back(ev->arg);
			}
		}
	}

	return 0;
}