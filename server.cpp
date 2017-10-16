#include <stdio.h>
#include <stdlib.h>	/* needed for os x*/
 #include<cstdlib>
#include <string.h>	/* strlen */
#include <netdb.h>      /* gethostbyname() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#define _GLIBCXX_USE_C99 1
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
		if (bytesRecvd >= 4) {
			myBuffer[bytesRecvd] = 0;//wth is this...
			std::string identifier(&myBuffer[0],&myBuffer[4]);//need to change to read until /r/l or something
			std::string remoteIPAddress=std::string (std::string (inet_ntoa(remoteAddress.sin_addr)));
			socklen_t remoteIPAddressSize = sizeof(remoteAddress);
			
			//std::cerr << "identifier: " <<identifier << "\n";
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
	     			newUser.myIPAddress = remoteIPAddress;//inet_ntoa(remoteAddress.sin_addr.s_addr);
	     			newUser.myChannels.push_back("Common");
	     			newUser.myCurrentChannel = "Common";
	     			currentUsers.push_back(newUser);
	     		}
	     		std::cerr << "currentUsers size: " << currentUsers.size() << std::endl;
			}
			else if (std::atoi(identifier.c_str()) == 1){//logout currently hacked, need to store ip
				if (currentUsers.size()>0) {
					int size = (int) currentUsers.size();
					for (int x=0; x <size; x++){
						if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
	     					std::cerr << "Logging out " <<currentUsers[x].myUserName << std::endl;
	     					currentUsers.erase(currentUsers.begin()+x);
	     					x = size;//end loop hack

	     				}

	     			}
	     		}
			}
			else if (std::atoi(identifier.c_str()) == 5){//list of channels
			

				std::string channelString="";
				//for (std::vector<std::string>::iterator iter = channelList.begin(); iter != channelList.end(); ++iter) {
	     		   
	     		  // sprintf(myBuffer[8+channels++*32], iter->c_str());
	     		//}
	     		std::cerr << "channel list request: "<< channelString <<std::endl;	
	     		int channelListSize = channelList.size();
				int thisBufSize = 4+4+32*channelListSize;
				char channelsBuffer[thisBufSize];//should be 4 + 4+32 * numchannels
				
				sprintf(channelsBuffer, "%s","0001");
				std::cerr << "number of channels: " <<channelListSize<<std::endl;
				//sprintf(myBuffer, "%s",std::to_string(numChannels.byte[3]).c_str());
				/*sprintf(myBuffer, numChannels.byte[2]);
				sprintf(myBuffer, numChannels.byte[1]);
				sprintf(myBuffer, numChannels.byte[0]);
				int channelNum = 0;
				for (int j=1; j<= numChannels.integer; j++){
					char temp[32];
					strcat(temp,channelList[j].c_str());
					//std::ostringstream out;
					//out << channelList[j];
					//std::string filename = out.str();
					//
					sprintf(myBuffer,"%s",temp);

				}*/

				for (std::vector<std::string>::iterator iter = channelList.begin(); iter != channelList.end(); ++iter) {
	     		   //std::cout<<*iter<<std::endl;
	     		   std::string temp = *iter;
	     		   sprintf(channelsBuffer, "%s",temp.c_str());
	     		}
	
	     		
				if (sendto(mySocket, channelsBuffer, strlen(channelsBuffer), 0, (struct sockaddr *)&remoteAddress, remoteIPAddressSize)==-1)
					perror("server sending channel list error");
				std::cerr << "Success.\n";



	    /* 		void requestChannels(int socket,sockaddr_in remoteAddress,int addressSize){
					char returnBuffer[4];
					sprintf(myBuffer, "0005");
					if (sendto(socket, returnBuffer, strlen(returnBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
						perror("sendto");
					std::cerr << "Success.\n";*/




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

