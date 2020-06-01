#include <iostream>
#include "talk_client.h"
using namespace std;


int main()
{
	TalkClient tc;
	tc.run();

	//string m_read_buf = "you say:\nhello word";
	//
	//size_t pos = m_read_buf.find("\n");
	//string person = m_read_buf.substr(0, pos);
	//string body = m_read_buf.substr(pos + 1, m_read_buf.size() - pos);
	//
	//cout << "person:\n" << person << endl;
	//cout << "body:\n" << body << endl;
	return 0;
}