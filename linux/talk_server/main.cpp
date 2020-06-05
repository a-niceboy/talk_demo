#include <sys/socket.h>	//socket bind listen accept
#include <arpa/inet.h>	//hton
#include <unistd.h>		//read write sleep

#include <sys/ioctl.h>	//ioctl 设置非阻塞
#include <sys/epoll.h>	//epoll

#include <fcntl.h>

#include <string.h>		//bzero
#include <errno.h>
#include <time.h>

#include <map>
#include <iostream>

#include "common.h"
using namespace std;

#define	MAX_EVENTS		1024
#define	BUFLEN			4096
#define	SERV_POST		6565

#define	ACCOUNT_SIGN		"account##"
#define	ROOM_ID_SIGN		"room_id##"
#define	CREATE_ROOM_SIGN	"create_room##"
#define	JOIN_ROOM_SIGN		"join_room##"
#define	TALK_DATA_SIGN		"talk_data##"

map<int, Talker> talker;
map<int, Room> room;

int get_room_id() 
{
	int i = 0;
	while (true)
	{
		map<int, Room>::iterator iter = room.find(i);
		if (iter == room.end())
		{
			break;
		}
		else
		{
			i++;
		}
	}
	return i;
}

/*描述就绪文件描述符相关信息*/
struct myevent_s
{
	int fd = 0;											//要监听的文件描述符
	int events = 0;										//对应的监听事件
	void* arg;											//泛型参数
	void (*call_back)(void* arg);						//回调函数

	int status = 0;										//是否在监听：1->在红黑树上（监听）， 0->不在（不监听）
	char buf[BUFLEN];									//数据
	int len = 0;
	long long last_active_time = 0;						//最后活跃时间
};

int g_efd;

struct myevent_s g_events[MAX_EVENTS + 1];

/*将结构体 myevect_s成员变量初始化*/
void eventset(struct myevent_s* ev, int fd, void (*call_back)(void*), void* arg)
{
	ev->fd = fd;
	ev->events = 0;
	ev->arg = arg;
	ev->call_back = call_back;

	ev->status = 0;
	bzero(ev->buf, sizeof(ev->buf));
	ev->len = 0;
	//服务端设置不用
	ev->last_active_time = time(NULL);
}

/*向 epoll监听的红黑树 添加一个 文件描述符*/
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

/*向 epoll监听的红黑树 删除一个 文件描述符*/
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

//客户端读回调函数
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

			map<int, Talker>::iterator iter = talker.find(ev->fd);
			if (iter != talker.end())
			{
				int room_id = iter->second._room_id;
				map<int, Room>::iterator room_iter = room.find(room_id);
				if (room_iter != room.end())
				{
					map<int, Talker>::iterator room_talker_iter = room_iter->second._talkers.find(ev->fd);
					if (room_talker_iter != room_iter->second._talkers.end())
					{
						room_iter->second._talkers.erase(room_talker_iter);
					}

					if (room_iter->second._talkers.size() == 0);
					room.erase(room_iter);
				}

				talker.erase(iter);
			}

			eventdel(g_efd, ev);
			close(ev->fd);
			return;
		}
		//用于非阻塞read
		else
		{
			//表示中断，处理
			if (errno == EINTR)
			{
				cerr << "read EINTR" << endl;
				continue;
			}
			//没有数据先结束稍后再说
			if (errno == EAGAIN)
			{
				cerr << "waiting..." << endl;
				break;
			}
		}
	}
	ev->buf[ev->len] = '\0';
	eventdel(g_efd, ev);

	string read_data = ev->buf;

	if (read_data.find(CREATE_ROOM_SIGN) != string::npos)
	{		
		int room_id = get_room_id();

		talker[ev->fd]._room_id = room_id;
		talker[ev->fd]._write_data = ROOM_ID_SIGN + to_string(room_id);
		
		Room temp_room;
		temp_room._talkers[ev->fd] = talker[ev->fd];
		room[room_id] = temp_room;
		
	}
	else if (read_data.find(ACCOUNT_SIGN) != string::npos)
	{
		size_t pos = read_data.find(ACCOUNT_SIGN);
		string temp = ACCOUNT_SIGN;
		string nick_name = read_data.substr(pos + temp.size(), read_data.size() - pos - temp.size());

		bool is_have = false;
		map<int, Room>::iterator room_iter =  room.find(talker[ev->fd]._room_id);
		if (room_iter == room.end())
		{
			cerr << "no room id" << endl;
		}
		else
		{
			for (map<int, Talker>::iterator iter = room_iter->second._talkers.begin(); iter != room_iter->second._talkers.end(); iter++)
			{
				if (iter->second._nick_name == nick_name)
				{
					is_have = true;
					break;
				}
			}
		}

		if (is_have)
		{
			talker[ev->fd]._write_data = "this room have name";
		}
		else
		{
			talker[ev->fd]._write_data = "ok";
			talker[ev->fd]._nick_name = nick_name;
		}	
	}
	else if (read_data.find(JOIN_ROOM_SIGN) != string::npos)
	{
		size_t pos = read_data.find(JOIN_ROOM_SIGN);
		string temp = JOIN_ROOM_SIGN;
		int room_id = atoi(read_data.substr(pos + temp.size(), read_data.size() - pos - temp.size()).c_str());
		map<int, Room>::iterator iter = room.find(room_id);
		if (iter == room.end())
		{
			talker[ev->fd]._write_data = "room no find";
		}
		else
		{
			talker[ev->fd]._room_id = room_id;
			iter->second._talkers[ev->fd] = talker[ev->fd];
			talker[ev->fd]._write_data = "ok";
		}
	}
	else if (read_data.find(TALK_DATA_SIGN) != string::npos)
	{
		size_t pos = read_data.find(TALK_DATA_SIGN);
		talker[ev->fd]._is_talking = true;
		string temp = TALK_DATA_SIGN;
		talker[ev->fd]._write_data = read_data.substr(pos + temp.size(), read_data.size() - pos - temp.size());
	}
	else
	{
		cout << "do error!!: " << endl;
	}

	eventset(ev, ev->fd, senddata, ev);
	eventadd(g_efd, (EPOLLOUT | EPOLLET), ev);

	cout << "====  recv success  ====" << endl;
}

//客户端写回调函数
void senddata(void* arg)
{
	cout << "==== send ..." << endl;
	struct myevent_s* ev = (struct myevent_s*)arg;

	if (!talker[ev->fd]._is_talking)
	{
		string out = talker[ev->fd]._write_data;
		int flag = send(ev->fd, out.c_str(), out.size(), 0);

		if (flag > 0)
		{
			eventdel(g_efd, ev);
			eventset(ev, ev->fd, recvdata, ev);
			eventadd(g_efd, (EPOLLIN | EPOLLET), ev);

			cout << "====  send success  ====" << endl;
		}
		else
		{
			cout << "client fd: " << ev->fd << "send error" << endl;
			map<int, Talker>::iterator iter = talker.find(ev->fd);
			if (iter != talker.end())
			{
				talker.erase(iter);
			}
			eventdel(g_efd, ev);
			close(ev->fd);
		}
	}
	else
	{
		time_t now_time = time(NULL);
		tm now_tm;
		localtime_r(&now_time, &now_tm);

		char szTime[50] = "";
		strftime(szTime, 50, "%Y-%m-%d %H:%M:%S", &now_tm);
		string out = szTime;
		out += " you say: \n";
		out += talker[ev->fd]._write_data;

		int flag = send(ev->fd, out.c_str(), out.size(), 0);

		if (flag > 0)
		{
			eventdel(g_efd, ev);
			eventset(ev, ev->fd, recvdata, ev);
			eventadd(g_efd, (EPOLLIN | EPOLLET), ev);

			cout << "====  send success  ====" << endl;
		}
		else
		{
			cout << "client fd: " << ev->fd << "send error" << endl;
			map<int, Talker>::iterator iter = talker.find(ev->fd);
			if (iter != talker.end())
			{
				talker.erase(iter);
			}
			eventdel(g_efd, ev);
			close(ev->fd);
		}

		int room_id = talker[ev->fd]._room_id;

		map<int, Room>::iterator room_iter = room.find(room_id);
		if (room_iter == room.end())
		{
			cerr << "no room id" << endl;
		}
		else
		{
			for (auto iter = room_iter->second._talkers.begin(); iter != room_iter->second._talkers.end(); )
			{
				if (iter->first == ev->fd)
				{
					cout << "is only" << endl;
					sleep(1);
					iter++;
					continue;
				}

				out = szTime;
				out += " " + talker[ev->fd]._nick_name + " say: \n";
				out += talker[ev->fd]._write_data;
				int flag = send(iter->first, out.c_str(), out.size(), 0);
				if (flag > 0)
				{
					cout << "====  send success  ====" << endl;
					iter++;
				}
				else
				{
					cerr << "client fd: " << ev->fd << "send error" << endl;
					iter = room_iter->second._talkers.erase(iter);					
					eventdel(g_efd, ev);
					close(ev->fd);
					
				}
			}
		}		
	}
}

//服务端回调函数
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
			/*出错*/
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

		Talker temp_talker;
		talker[ev->fd] = temp_talker;
		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		eventset(&g_events[i], client_fd, recvdata, &g_events[i]);
		eventadd(g_efd, (EPOLLIN | EPOLLET), &g_events[i]);

	} while (0);

	cout << "====  connect success  ====" << endl;
	cout << "\n" << endl;
}

void init_listen_socket(int efd, short port)
{
	cout << "====  server init ... " << endl;

	int lfd = socket(AF_INET, SOCK_STREAM, 0);

	// 设置 地址复用
	socklen_t val = 1;
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
	{
		cerr << "setsockopt  error : " << endl;
		close(lfd);
		return;
	}
	//非阻塞
	fcntl(lfd, F_SETFL, O_NONBLOCK);

	//eventset(struct myevent_s *ev , lfd, void(*call_back)(int,int,void*), void* arg));
	eventset(&g_events[MAX_EVENTS], lfd, accept_conn, &g_events[MAX_EVENTS]);
	eventadd(efd, (EPOLLIN | EPOLLET), &g_events[MAX_EVENTS]);

	//绑定信息
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

	cout << "=======  server init end  ======= " << endl;
	cout << "\n" << endl;
}

int main(int argc, char* argv[])
{
	Room normal_room;
	room[0] = normal_room;

	cout << "========  hello  ========" << endl;
	cout << "=====  talk server  =====" << endl;

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
		//活跃超时去掉
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
			if (duration >= 6000)
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