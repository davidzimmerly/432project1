#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>	/* strlen, memset */
#include <iostream>
#include <unistd.h> /* close */
#include <vector>
#include <termios.h>
#include <sstream>
#include <sys/select.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros


const int loginSize = 36;
const int joinLeaveWhoSize = 36;
const int sayRequestSize = 100;
const int saySize = 132;
const int logoutListKeepAliveSize = 4;
const int maxConnections = 256;
const int errorSize = 68;
#include "duckchat.h"

#define THEPORT  3264
#define BUFFERLENGTH  1024

struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myActiveChannel;
	std::string myIPAddress;
	int myPort;
	time_t lastSeen;
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
	std::vector<std::string> myUsers;
	
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
