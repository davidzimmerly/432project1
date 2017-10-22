const uint8_t loginSize = 36;
const uint8_t logoutSize = 4;
const uint8_t joinSize = 36;//leave 
const uint8_t sayRequestSize = 100;
const uint8_t requestChannelSize = 4;
#include "duckchat.h"

struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myActiveChannel;
	std::string myIPAddress;
};

union intOrBytes
{
	uint32_t integer;
	unsigned char byte[4];
};

void initBuffer(char* buf, unsigned int length){
	for (uint32_t x=0; x<length ; x++){
		buf[x]='\0';
	}
}

uint8_t findStringEnd(char* myBuffer,uint8_t start,uint8_t finish){
	uint8_t end=0;
	for (uint8_t x=4;x<36;x++){
		if (myBuffer[x]=='\0'){
			end = x;
			break;
		}
	}
	return end;
}
