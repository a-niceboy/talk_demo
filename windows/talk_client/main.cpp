#include <iostream>
#include "talk_client.h"
using namespace std;

int main()
{
	TalkClient tc;
	tc.init();
	tc.run();

	return 0;
}