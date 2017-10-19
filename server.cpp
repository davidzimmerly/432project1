#include <stdio.h>
#include <stdlib.h>	/* needed for os x*/
#include <cstdlib>
#include <cstdint>
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
#include "shared.h"
#define BUFFERLENGTH  1024
#define THEPORT  3264


class server{
	private:
		struct sockaddr_in myAddress;
		struct sockaddr_in remoteAddress;
		uint32_t bytesRecvd;
		socklen_t addressSize;
		int mySocket;
		std::vector<userInfo> currentUsers;
		std::vector<std::string> channelList;
			
		void serve(){
			while(1) {
				unsigned char myBuffer[BUFFERLENGTH];
				initBuffer(myBuffer,BUFFERLENGTH);
				
				printf("waiting on port %d\n", THEPORT);
				bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
				printf("received %d bytes\n", bytesRecvd);
				if (bytesRecvd >= 4) {
					std::string identifier(&myBuffer[0],&myBuffer[4]);//need to change to read until /r/l or something
					std::string remoteIPAddress=std::string (std::string (inet_ntoa(remoteAddress.sin_addr)));
					socklen_t remoteIPAddressSize = sizeof(remoteAddress);
					if (std::atoi(identifier.c_str()) == 0){//login
						if (bytesRecvd==36){//valid login packet size
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
				     			newUser.myIPAddress = remoteIPAddress;
				     			newUser.myChannels.push_back("Common");
				     			newUser.myActiveChannel = "Common";
				     			currentUsers.push_back(newUser);
				     		}
				     		std::cerr << "currentUsers size: " << currentUsers.size() << std::endl;
				     	}
					}
					else if (std::atoi(identifier.c_str()) == 1){//logout
						if (currentUsers.size()>0) {
							uint32_t size =  currentUsers.size();
							for (uint32_t x=0; x <size; x++){
								if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
			     					std::cerr << "Logging out " <<currentUsers[x].myUserName << std::endl;
			     					currentUsers.erase(currentUsers.begin()+x);
			     					x = size;//end loop hack

			     				}

			     			}
			     		}
					}
					else if (std::atoi(identifier.c_str()) == 2){//join request
						std::string channelNameBuffer(&myBuffer[4],&myBuffer[36]);
						std::string channelToJoin="";
						std::string userName;
						uint32_t size =  currentUsers.size();
						bool userFound = false;
						uint32_t userSlot = 0;
						for (uint32_t x=0; x <size; x++){
							if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
		     					userFound=true;
		     					userName = currentUsers[x].myUserName;
		     					userSlot = x;
		     					x = size;//end loop hack
		     				}

		     			}
		     			if (userFound){
		     				
		     				uint8_t end=0;
			     			for (uint8_t x=4;x<36;x++){
			     				if (channelNameBuffer[x]=='\0'){
			     					end = x;
			     					x=36;
			     				}
			     			}
			     			if (end>0)
			     				channelToJoin = channelNameBuffer.substr(0,36-end);

			     			std::cerr << "join request received from "<< "userName: " <<userName << "for channel " << channelToJoin << std::endl;
							bool channelFound=false;
							for (uint32_t x=0; x <size; x++){
							
								if (channelToJoin.compare(channelList[x]) == 0){
			     					channelFound=true;
			     					x = size;//end loop hack
			     				}

			     			}
			     			if (!channelFound){
			     				channelList.push_back(channelToJoin);
			     				currentUsers[userSlot].myActiveChannel = channelToJoin;
			     			}
		     				


		     			}

						
						

					}
					else if (std::atoi(identifier.c_str()) == 5){//list of channels
						union intOrBytes channelListSize;
			     		channelListSize.integer = channelList.size();
						uint64_t thisBufSize = 4+4+32*channelListSize.integer;
						unsigned char channelsBuffer[thisBufSize];//should be 4 + 4+32 * numchannels...? need to fix this list per spec
						for (uint64_t x=0; x<thisBufSize; x++){
		     		   		channelsBuffer[x]='\0';
		     		   	}   



						std::cerr << "list requested, number of channels: " <<channelListSize.integer<<std::endl;
						//std::string output = "";
						//strcpy(channelsBuffer,"0001");
						channelsBuffer[0] = channelsBuffer[1] = channelsBuffer[2] = '0';
						channelsBuffer[3] = '1';
						

						channelsBuffer[4] = channelListSize.byte[0];
						channelsBuffer[5] = channelListSize.byte[1];
						channelsBuffer[6] = channelListSize.byte[2];
						channelsBuffer[7] = channelListSize.byte[3];
						uint8_t position = 8;
						
						 						
						for (std::vector<std::string>::iterator iter = channelList.begin(); iter != channelList.end(); ++iter) {
							uint8_t x=0;		 
							std::string theCopy = *iter;
			     		    for (x=0; x<theCopy.length(); x++){//need error checking on input eventually, assuming it is <=32 here
			     		    	channelsBuffer[position+x] = theCopy[x];
			     		    }   
			     		    position+=32;
			     		
			     		}
						if (sendto(mySocket, channelsBuffer, thisBufSize, 0, (struct sockaddr *)&remoteAddress, remoteIPAddressSize)==-1)
							perror("server sending channel list error");
						std::cerr << "Success.\n";
					}
					

					//show who is online
					for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
		     		   std::cerr << (*iter).myUserName << "is Online in channel "<< iter->myActiveChannel << " at IP " << iter->myIPAddress<<std::endl;
		     		}
		     		if (currentUsers.size()==0)
		     			std::cerr << "No One is Currently Online!" << std::endl;
				}
			}
		}
		
	public:
		server(){
			addressSize = sizeof(myAddress);
			bytesRecvd=0;
			mySocket=socket(AF_INET, SOCK_DGRAM, 0);
			if (mySocket<0) {
				perror ("socket() failed");
				exit(1);
			}
			channelList.push_back("Common");
			channelList.push_back("strange");
			channelList.push_back("rare");
			memset((char *)&myAddress, 0, sizeof(myAddress));
			myAddress.sin_family = AF_INET;
			myAddress.sin_addr.s_addr = htonl(INADDR_ANY);
			myAddress.sin_port = htons(THEPORT);
			printf("%s\n",inet_ntoa(myAddress.sin_addr));
			if (bind(mySocket, (struct sockaddr *)&myAddress, sizeof(myAddress)) < 0) {
				perror("bind failed");
				exit(1);
			}
			else
				serve();
		}
			
		

};


int main (int argc, char *argv[]){
	server* myServer = new server();
	delete(myServer);
	return 0;
}

