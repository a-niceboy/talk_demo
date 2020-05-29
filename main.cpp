#include <sys/socket.h>	//socket bind listen accept
#include <arpa/inet.h>	//hton
#include <unistd.h>		//read write sleep

#include <sys/ioctl.h>	//ioctl ���÷�����
#include <sys/epoll.h>	//epoll

#include <fcntl.h>

#include <string.h>		//bzero
#include <errno.h>
#include <iostream>

using namespace std;
#define	MAX_EVENTS		1024
#define	BUFLEN			4096
#define	SERV_POST		6565

/*���������ļ������������Ϣ*/

struct myevent_s
{
	int fd = 0;											//Ҫ�������ļ�������
	int events = 0;										//��Ӧ�ļ����¼�
	void* arg;											//���Ͳ���
	void (*call_back)(int fd, int events, void* arg);	//�ص�����
	
	int status = 0;										//�Ƿ��ڼ�����1->�ں�����ϣ��������� 0->���ڣ���������
	char buf[BUFLEN];									//����
	int len = 0;
	long long last_active_time = 0;						//����Ծʱ��
};

int g_efd;

struct myevent_s g_events[MAX_EVENTS + 1];

/*���ṹ�� myevect_s��Ա������ʼ��*/
void eventset(struct myevent_s* ev, int fd, void (*call_back)(int, int, void*), void* arg)
{
	cout << "eventset" << endl;
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
	cout << "eventadd" << endl;
	int op;
	if (ev->status == 1)
	{
		op = EPOLL_CTL_MOD;
	}
	else
	{
		op = EPOLL_CTL_ADD;
	}

	struct epoll_event epv = { 0, {0} };

	epv.data.ptr = ev;
	epv.events = ev->events = events;

	if (epoll_ctl(efd, op, ev->fd, &epv) < 0)
	{
		//error
	}
	else
	{
		//add ok
	}
}

/*�� epoll�����ĺ���� ɾ��һ�� �ļ�������*/
void eventdel(int efd, struct myevent_s* ev)
{
	cout << "eventdel" << endl;
	if (ev->status != 1)
	{
		return;
	}

	struct epoll_event epv = { 0, {0} };

	epv.data.ptr = ev;
	ev->status = 0;

	epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);
}

void recvdata(int fd, int events, void* arg);
//�ͻ���д�ص�����
void senddata(int fd, int events, void* arg)
{
	cout << "senddata" << endl;
	struct myevent_s* ev = (struct myevent_s*)arg;
	int len = send(fd, ev->buf, ev->len, 0);

	if (len > 0)
	{
		eventdel(g_efd, ev);
		eventset(ev, fd, recvdata, ev);
		eventadd(g_efd, EPOLLIN, ev);
	}
	else
	{
		close(fd);
		eventdel(g_efd, ev);
	}
}

//�ͻ��˶��ص�����
void recvdata(int fd, int events, void* arg)
{
	cout << "recvdata" << endl;
	struct myevent_s* ev = (struct myevent_s*)arg;
	int len = recv(fd, ev->buf, sizeof(ev->buf), 0);
	eventdel(g_efd, ev);

	if (len > 0)
	{
		ev->len = len;
		ev->buf[len] = '\0';

		/*
		����
		*/
		cout << "buf: " << ev->buf << endl;
		eventset(ev, fd, senddata, ev);
		eventadd(g_efd, EPOLLOUT, ev);
	}
	else if (len == 0)
	{
		close(ev->fd);
	}
	else
	{
		close(ev->fd);
	}

}

//����˻ص�����
void accept_conn(int lfd, int events, void* arg)
{
	cout << "accept_conn" << endl;
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	int i = 0;

	int client_fd = accept(lfd, (struct sockaddr*) & client_addr, &len);
	if (client_fd < 0)
	{
		if (errno != EAGAIN && errno != EINTR)
		{
			/*����*/
		}
		return;
	}

	do
	{
		for (i = 0; i < MAX_EVENTS; i++)
		{
			cout << "i: " << i << endl;
			if (g_events[i].status == 0)
			{
				break;
			}
		}
		if (i == MAX_EVENTS)
		{
			//max connect
			break;
		}
		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		eventset(&g_events[i], client_fd, recvdata, &g_events[i]);
		eventadd(g_efd, EPOLLIN, &g_events[i]);

	} while (0);

	cout << "client connect" << endl;
	//client connect

}

void init_listen_socket(int efd, short port)
{
	cout << "init_listen_socket" << endl;
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//������
	fcntl(lfd, F_SETFL, O_NONBLOCK);

	//eventset(struct myevent_s *ev , lfd, void(*call_back)(int,int,void*), void* arg));
	eventset(&g_events[MAX_EVENTS], lfd, accept_conn, &g_events[MAX_EVENTS]);

	eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);

	//����Ϣ
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(lfd, (sockaddr*)&addr, sizeof(addr));

	listen(lfd, 128);
}

int main(int argc, char* argv[])
{
	cout << "main" << endl;
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

			if (duration >= 60)
			{
				close(g_events[check_pos].fd);
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
			myevent_s* ev = (myevent_s *)events[i].data.ptr;

			if ((events[i].events & EPOLLIN) &&
				(ev->events & EPOLLIN))
			{
				ev->call_back(ev->fd, events[i].events, ev->arg);
			}
			if ((events[i].events & EPOLLOUT) &&
				(ev->events & EPOLLOUT))
			{
				ev->call_back(ev->fd, events[i].events, ev->arg);
			}
		}
	}

	return 0;
}