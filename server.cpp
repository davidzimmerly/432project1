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
		int bytesRecvd;
		socklen_t addressSize;
		int mySocket;
		std::vector<userInfo> currentUsers;
		std::vector<std::string> channelList;
		int findUserSlot(std::string remoteIPAddress){
			int userSlot=-1;
			int size=currentUsers.size();
			
			for (int x=0; x <size; x++){
				if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
					userSlot = x;
					break;
				}

			}
			return userSlot;
		     			
		}

		void serve(){
			while(true) {
				bytesRecvd = 0;
				char myBuffer[BUFFERLENGTH];
				//initBuffer(myBuffer,BUFFERLENGTH);
				
				printf("waiting on port %d\n", THEPORT);
				bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
				printf("received %d bytes\n", bytesRecvd);
				if (bytesRecvd >= 4) {
					struct request* incoming_request = new request;
					incoming_request = (struct request*)myBuffer;
					request_t identifier = incoming_request->req_type;

					/*std::string id(&myBuffer[0],&myBuffer[4]);//need to change to read until /r/l or something
					int identifier = std::atoi(id.c_str());*/
					std::string remoteIPAddress=std::string (std::string (inet_ntoa(remoteAddress.sin_addr)));
					socklen_t remoteIPAddressSize = sizeof(remoteAddress);
					std::cerr << "request type " << identifier <<std::endl;
					if (identifier == REQ_LOGIN){//login
						if (bytesRecvd==loginSize){//checks valid login packet size
							struct request_login* incoming_login_request;
							incoming_login_request = (struct request_login*)myBuffer;

							std::string userName = std::string(incoming_login_request->req_username);
							
							//std::cerr << "from username " << incoming_login_request->req_username <<std::endl;
							std::cerr << "from username " << userName <<std::endl;

							//std::cerr << "A" << std::endl;
							//int end = findStringEnd(myBuffer,4,35);
							//std::cerr << "B end=" <<end<< std::endl;
							//std::string userName(&myBuffer[4],&myBuffer[end]);
							//std::cerr << "login request received from "<< "userName: " <<userName << std::endl;
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
				     		
				     		//std::cerr << "currentUsers size: " << currentUsers.size() << std::endl;
				     	}
					}
					else if (identifier == REQ_LOGOUT && bytesRecvd==4){//logout
						if (currentUsers.size()>0) {
							int size =  currentUsers.size();
							for (int x=0; x <size; x++){
								if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
			     					//std::cerr << "Logging out " <<currentUsers[x].myUserName << std::endl;
			     					currentUsers.erase(currentUsers.begin()+x);
			     					//currentUsers.erase(x,x);
			     					x = size;//end loop hack
			     				}
			     			}
			     		}
					}
					else if (identifier == 2 && bytesRecvd == joinSize){//join request
						std::string channelNameBuffer(&myBuffer[4],&myBuffer[36]);
						std::string channelToJoin="";
						std::string userName;
						int size =  currentUsers.size();
						int userSlot = findUserSlot(remoteIPAddress);

		     			if (userSlot>=0){//user found
		     				userName = currentUsers[userSlot].myUserName;
		     					
		     				int end=0;
			     			for (int x=4;x<36;x++){
			     				if (channelNameBuffer[x]=='\0'){
			     					end = x;
			     					break;
			     				}
			     			}
			     			if (end>0)
			     				channelToJoin = channelNameBuffer.substr(0,end);

			     			//std::cerr << "join request received from "<< "userName: " <<userName << "for channel " << channelNameBuffer << "."<<std::endl;
							bool channelFound=false;
							for (unsigned int x=0; x <channelList.size(); x++){
								//std::cerr << "we compare channels : "<< channelToJoin << " with " << channelList[x]<<":"<<std::endl;
								if (channelToJoin.compare(channelList[x]) == 0){
			     					//std::cerr << "Found channel "<< channelToJoin<< " already here: "<<channelList[x]<<"."<<std::endl;
			     					channelFound=true;
			     					break;
			     				}
							}
			     			if (!channelFound){//if channel not exist, add it
			     				channelList.push_back(channelToJoin);
			     				std::cerr << "adding a new channel to server list: "<<channelToJoin<<std::endl;
			     			}

			     			channelFound=false;
			     			//now add to user info
			     			size = currentUsers[userSlot].myChannels.size();
			     			for (int x=0; x <size; x++){
								if (channelToJoin.compare(currentUsers[userSlot].myChannels[x]) == 0){
			     					channelFound=true;
			     					break;
			     				}
							}
							if (!channelFound){//if channel not exist in user info, add it
			     				currentUsers[userSlot].myChannels.push_back(channelToJoin);
			     			}
			     			
			     			
			     			
		     				currentUsers[userSlot].myActiveChannel = channelToJoin;
						}
					}
					else if (identifier == 3){//leave request
						std::string channelNameBuffer(&myBuffer[4],&myBuffer[36]);
						std::string channelToLeave="";
						std::string userName;
						int userSlot = findUserSlot(remoteIPAddress);

		     			if (userSlot>=0){//user found
		     				//channelToLeave = std::string(channelNameBuffer);
		     				int end=0;
			     			for (int x=4;x<36;x++){
			     				if (channelNameBuffer[x]=='\0'){
			     					end = x;
			     					x=36;
			     				}
			     			}
			     			if (end>0)
			     				channelToLeave = channelNameBuffer.substr(0,end);
			     			
		     				std::cerr << "leave request received from "<< "userName: " <<userName << "for channel " << channelToLeave << std::endl;



							for (unsigned int x=0; x <currentUsers[userSlot].myChannels.size(); x++){
								if (channelToLeave.compare(currentUsers[userSlot].myChannels[x]) == 0){
			     					currentUsers[userSlot].myChannels.erase(currentUsers[userSlot].myChannels.begin() + x);
			     					std::cerr << "successfully left channel "<<channelToLeave<<std::endl;
			     					break;
			     				}
			     			}
		     				if (currentUsers[userSlot].myActiveChannel == channelToLeave)
		     					currentUsers[userSlot].myActiveChannel = "Common";
		     			}
					}
					else if (identifier == 4){//say request
						std::string channelNameBuffer(&myBuffer[4],&myBuffer[36]);
						std::string textFieldBuffer(&myBuffer[37],&myBuffer[99]);
						std::string channelToAnnounce="";
						std::string textField="";
						std::string userName;
						int userSlot = findUserSlot(remoteIPAddress);

		     			if (userSlot>=0){//user found
		     				int end=0;
			     			for (int x=4;x<36;x++){
			     				if (channelNameBuffer[x]=='\0'){
			     					end = x;
			     					x=36;
			     				}
			     			}
			     			end=0;

			     			for (int x=4;x<36;x++){
			     				if (channelNameBuffer[x]=='\0'){
			     					end = x;
			     					x=36;
			     				}
			     			}
			     			
			     			if (end>0)
			     				channelToAnnounce = channelNameBuffer.substr(0,end);
			     			end=0;
			     			for (int x=4;x<36;x++){
			     				if (channelNameBuffer[x]=='\0'){
			     					end = x;
			     					x=36;
			     				}
			     			}
			     			if (end>0)
			     				textField = textFieldBuffer.substr(0,end);




			     			std::cerr << "say request received from "<< "userName: " <<userName << "for channel " << channelToAnnounce << std::endl;
			     			std::cerr <<"msg: "<< textField << std::endl;
			     			//now need to find all users of this channel (maybe a function for this?)


							/*for (int x=0; x <currentUsers[userSlot].myChannels.size(); x++){
								if (channelToLeave.compare(currentUsers[userSlot].myChannels[x]) == 0){
			     					currentUsers[userSlot].myChannels.erase(currentUsers[userSlot].myChannels.begin() + x);
			     					std::cerr << "successfully left channel "<<channelToLeave<<std::endl;
			     					break;
			     				}
			     			}
		     				if (currentUsers[userSlot].myActiveChannel == channelToLeave)
		     					currentUsers[userSlot].myActiveChannel = "Common";*/
		     			}
					}
					
					else if (identifier == 5){//list of channels
						union intOrBytes channelListSize;
			     		channelListSize.integer = channelList.size();
						int thisBufSize = 4+4+32*channelListSize.integer;
						 char channelsBuffer[thisBufSize];//should be 4 + 4+32 * numchannels...? need to fix this list per spec
						initBuffer(channelsBuffer,thisBufSize);
						std::cerr << "list requested, number of channels: " <<channelListSize.integer<<std::endl;
						channelsBuffer[0] = channelsBuffer[1] = channelsBuffer[2] = '0';
						channelsBuffer[3] = '1';
						channelsBuffer[4] = channelListSize.byte[0];
						channelsBuffer[5] = channelListSize.byte[1];
						channelsBuffer[6] = channelListSize.byte[2];
						channelsBuffer[7] = channelListSize.byte[3];
						int position = 8;
						for (std::vector<std::string>::iterator iter = channelList.begin(); iter != channelList.end(); ++iter) {
							unsigned int x=0;		 
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
		     		   std::cerr << (*iter).myUserName << " is Online in channel "<< iter->myActiveChannel << " at IP " << iter->myIPAddress<<std::endl;
		     		   std::cerr << (*iter).myUserName << "'s channels: ";
		     		   for (std::vector<std::string>::iterator iter2 = (*iter).myChannels.begin(); iter2 != (*iter).myChannels.end(); ++iter2) {
		     		   		std::cerr << (*iter2) <<", ";
		     		   		
		     		   }
		     		   std::cerr<<std::endl;
		     		
		     		
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