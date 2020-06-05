#ifndef __USER_H__
#define __USER_H__

#include <string>
#include <map>


struct Talker
{
	Talker();
	//昵称
	std::string _nick_name;
	bool _is_talking;
	std::string _read_data;
	std::string _write_data;
	//房间号
	int _room_id;
	//0 汉语, 1 english
	int _language;
};

struct Room
{
	//用户
	std::map<int, Talker> _talkers;
};

#endif