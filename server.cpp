#include <stdio.h>
#include <stdlib.h>	/* needed for os x*/
 #include<cstdlib>
#include <string.h>	/* strlen */
#include <netdb.h>      /* gethostbyname() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <vector>

#define BUFFERLENGTH  1024
#define THEPORT  3264

struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myCurrentChannel;
	std::string myIPAddress;
};

int main (int argc, char *argv[]){
	struct sockaddr_in myAddress;
	struct sockaddr_in remoteAddress;
	socklen_t addressSize = sizeof(myAddress);
	int bytesRecvd;
	int mySocket;
	mySocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (mySocket<0) {
		perror ("socket() failed");
		return 0;
	}

	memset((char *)&myAddress, 0, sizeof(myAddress));
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddress.sin_port = htons(THEPORT);
	printf("%s\n",inet_ntoa(myAddress.sin_addr));
	std::vector<userInfo> currentUsers;
	std::vector<std::string> channelList;
	channelList.push_back("Common");
	
	if (bind(mySocket, (struct sockaddr *)&myAddress, sizeof(myAddress)) < 0) {
		perror("bind failed");
		return 0;
	}
	
	while(1) {
		unsigned char myBuffer[BUFFERLENGTH];
		printf("waiting on port %d\n", THEPORT);
		bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
		printf("received %d bytes\n", bytesRecvd);
		if (bytesRecvd > 4) {
			myBuffer[bytesRecvd] = 0;
			std::string identifier(&myBuffer[0],&myBuffer[4]);
			
			std::cerr << "identifier: " <<identifier << "\n";
			
			if (std::atoi(identifier.c_str()) == 0){//login
				std::string userName(&myBuffer[4],&myBuffer[35]);
				std::cerr << "login request received from "<< "userName: " <<userName << std::endl;
				bool addUser = true;
				if (currentUsers.size()>0) {
					for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
	     				if (userName.compare((*iter).myUserName) == 0){
	     					std::cerr<< " error , user is already logged in.";
	     					addUser = false;
	     				}
	     			}
	     		}
	     		if (addUser){
	     			std::cerr << "adding user" << std::endl;
	     			userInfo newUser;
	     			newUser.myUserName = userName;
	     			newUser.myIPAddress = remoteAddress.sin_addr.s_addr;//***hmm dunno if right 
	     			newUser.myChannels.push_back("Common");
	     			newUser.myCurrentChannel = "Common";
	     			currentUsers.push_back(newUser);
	     		}
	     		std::cerr << "currentUsers size: " << currentUsers.size() << std::endl;
			}
			if (std::atoi(identifier.c_str()) == 1){//logout currently hacked, need to store ip
				std::string userName(&myBuffer[4],&myBuffer[35]);
				int removeSlot = -1;
				if (currentUsers.size()>0) {
					for (int x=0; x <currentUsers.size(); x++){
						if (userName.compare(currentUsers[x].myUserName) == 0){
	     					currentUsers.erase(currentUsers.begin()+x);
	     					x = currentUsers.size();//end loop hack
	     				}

	     			}
	     		}
			}
			//show who is online
			for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
     		   std::cerr << (*iter).myUserName << "is Online in channel "<< iter->myCurrentChannel << " at IP " << iter->myIPAddress<<std::endl;
     		}
     		if (currentUsers.size()==0)
     			std::cerr << "No One is Currently Online!" << std::endl;
		}
	}
	return 0;
}

