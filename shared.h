#include <sys/socket.h>
#include <arpa/inet.h> /* htons */
#include <string>
#include <string.h>	/* strcopy */
#include <iostream>
#include <unistd.h> /* close */
#include <vector>
#include <termios.h>
#include <sstream>
#include <sys/select.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include "duckchat.h"
#include <fcntl.h> //open
//cis432 fall 2017 David Zimmerly
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


const int errorSize = 68;
const int keepAliveTimeout = 60;
const int serverTimeout = 120;
const int clientResponseWaitTime = 5;
const int loginSize = sizeof(struct request_login);//36;
const int joinLeaveWhoSize = sizeof(struct request_join);//36;
const int sayRequestSize = sizeof(struct request_say);//100;
const int saySize = sizeof(struct text_say);//132;
const int logoutListKeepAliveSize = sizeof(struct request_logout);//4;
const int s2sJoinLeaveSize = sizeof(struct request_s2s_join);//36;
const int s2sSaySize = sizeof(struct request_s2s_say);//140;
#define BUFFERLENGTH 65536

void initBuffer(char* buf,int size){ for (int x=0;x<size;x++){ buf[x]='\0'; } }
struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myActiveChannel;
	std::string myIPAddress;
	int myPort;
	time_t lastSeen;
};
struct serverInfo
{
	std::string myIPAddress;
	int myPort;
	std::vector<std::string> myChannels;
	std::vector<time_t> myTimeStamps;
	struct sockaddr_in myAddress;
};
struct requestIDInfo
{
	char id[8];
	time_t timeStamp;
};





int findStringPositionInVector(std::vector<std::string> inputV, std::string inputS){
	int found = -1;
	for (unsigned int x=0; x<inputV.size(); x++) {
		if (inputV[x]==inputS){
			found=x;
			break;
		}
	}
	return found;
}		
struct channelInfo
{
	std::string myChannelName;
	std::vector<struct userInfo> myUsers;
};
int findChannelInfoPositionInVector(std::vector<channelInfo> inputV, std::string inputS){
	int found = -1;
	for (unsigned int x=0; x<inputV.size(); x++) {
		if (inputV[x].myChannelName==inputS){
			found=x;
			break;
		}
	}
	return found;
}		
int findUserInfoPositionInVector(std::vector<userInfo> inputV, std::string inputS){
	int found = -1;
	for (unsigned int x=0; x<inputV.size(); x++) {
		if (inputV[x].myUserName==inputS){
			found=x;
			break;
		}
	}
	return found;
}		
		

void truncate(std::string& input, unsigned int max){
	if (input.length()>max) //format input if too big
		input = input.substr(0,max);
}
void setTimeout(int mySocket, int seconds){
	struct timeval timeOut;
	timeOut.tv_sec = seconds;
	timeOut.tv_usec = 0;
	int error1 = setsockopt(mySocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeOut,sizeof(struct timeval));
	if (error1<0){
		std::cerr<<"setsock error";
		exit(EXIT_FAILURE);
	}
}
static struct termios oldterm;
/* Returns -1 on error, 0 on success */
int raw_mode (void)
{
    struct termios term;
	if (tcgetattr(STDIN_FILENO, &term) != 0) return -1;
	oldterm = term;     
    term.c_lflag &= ~(ECHO);    /* Turn off echoing of typed charaters */
    term.c_lflag &= ~(ICANON);  /* Turn off line-based input */
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &term);
    return 0;
}
void cooked_mode (void)    
{   
    tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
}

struct listIDInfo
{
	char id[8];
	bool origin;
	
	//if origin these will be client's ip:port, else the server you got it from's 
	

	struct sockaddr_in remoteAddress;
	std::string remoteIPAddress;
	int remotePort;
	
	time_t timeStamp;//purge every 2 min during check like other request iDs
	std::vector<std::string> channels;
	int received; //if this equals numNeighbors then send to either origin client as text or origin server as s2s
};