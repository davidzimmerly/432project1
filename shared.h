#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>	/* strlen, memset */
#include <iostream>
#include <unistd.h> /* close */
#include <vector>

const int loginSize = 36;
const int joinLeaveWhoSize = 36;
const int sayRequestSize = 100;
const int logoutListSize = 4;
#include "duckchat.h"
#define THEPORT  3264
#define BUFFERLENGTH  1024

struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myActiveChannel;
	std::string myIPAddress;
};


struct channelInfo
{
	std::string myChannelName;
	std::vector<std::string> myUsers;
	
};

void truncate(std::string& input, unsigned int max){

	if (input.length()>max) //format input if too big
		input = input.substr(0,max);
}
