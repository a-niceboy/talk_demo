#ifndef __USER_H__
#define __USER_H__

#include <string>
#include <map>


struct Talker
{
	Talker();
	//�ǳ�
	std::string _nick_name;
	bool _is_talking;
	std::string _read_data;
	std::string _write_data;
	//�����
	int _room_id;
	//0 ����, 1 english
	int _language;
};

struct Room
{
	//�û�
	std::map<int, Talker> _talkers;
};

#endif