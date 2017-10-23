const int loginSize = 36;
const int logoutSize = 4;
const int joinLeaveSize = 36;
const int sayRequestSize = 100;
const int requestChannelSize = 4;
#include "duckchat.h"

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


union intOrBytes
{
	int integer;
	unsigned char byte[4];
};

void initBuffer(char* buf, int length){
	for (int x=0; x<length ; x++){
		buf[x]='\0';
	}
}

int findStringEnd(char* myBuffer,int start,int finish){
	int end=0;
	for (int x=start;x<finish;x++){
		if (myBuffer[x]=='\0'){
			end = x;
			break;
		}
	}
	return end;
}
