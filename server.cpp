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
#define BUFFERLENGTH  1024
#define THEPORT  3264

struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myCurrentChannel;
	std::string myIPAddress;
};

union intOrBytes
{
	unsigned int integer;
	unsigned char byte[4];
};

class server{
	private:
		struct sockaddr_in myAddress;
		struct sockaddr_in remoteAddress;
		int bytesRecvd;
		socklen_t addressSize;
		int mySocket;
		std::vector<userInfo> currentUsers;
		std::vector<std::string> channelList;
		void nullfill(char* input, int size){
			for (int x=0;x<size;x++){
					input[x] ='\0';
			}	
			
		}
			
		void serve(){
			while(1) {
				unsigned char myBuffer[BUFFERLENGTH];
				for (int x=0;x<BUFFERLENGTH;x++){
					myBuffer[x] ='\0';
				}	
			
				printf("waiting on port %d\n", THEPORT);
				bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
				printf("received %d bytes\n", bytesRecvd);
				if (bytesRecvd >= 4) {
					//myBuffer[bytesRecvd] = 0;//wth is this...
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
					else if (std::atoi(identifier.c_str()) == 1){//logout
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
					else if (std::atoi(identifier.c_str()) == 2){//join request
						std::string channelNameBuffer(&myBuffer[4],&myBuffer[36]);
						std::string channelToJoin="";
						std::string userName;
						int size = (int) currentUsers.size();
						bool userFound = false;
						int userSlot = 0;
						for (int x=0; x <size; x++){
							if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
		     					userFound=true;
		     					userName = currentUsers[x].myUserName;
		     					userSlot = x;
		     					x = size;//end loop hack
		     				}

		     			}
		     			if (userFound){
		     				
		     				int end=0;
			     			for (int x=4;x<36;x++){
			     				if (channelNameBuffer[x]=='\0'){
			     					end = x;
			     					x=36;
			     				}
			     			}
			     			if (end>0)
			     				channelToJoin = channelNameBuffer.substr(0,36-end);


			     			std::cerr << "channelNameBuffer " << channelNameBuffer << std::endl;		
			     			

		     				std::cerr << "join request received from "<< "userName: " <<userName << std::endl;		
		     				std::cerr << "for channel " << channelToJoin << std::endl;		
							bool channelFound=false;
							for (int x=0; x <size; x++){
							
								if (channelToJoin.compare(channelList[x]) == 0){
			     					channelFound=true;
			     					x = size;//end loop hack
			     				}

			     			}
			     			if (!channelFound){
			     				channelList.push_back(channelToJoin);
			     				currentUsers[userSlot].myCurrentChannel = channelToJoin;
			     			}
		     				


		     			}

						
						

						/*std::string channelString="";
						std::cerr << "channel list request: "<< channelString <<std::endl;	
			     		int channelListSize = channelList.size();
						int thisBufSize = 4+4+32*channelListSize;
						char channelsBuffer[thisBufSize];//should be 4 + 4+32 * numchannels
						sprintf(channelsBuffer, "%s","0001");
						std::cerr << "number of channels: " <<channelListSize<<std::endl;
						for (std::vector<std::string>::iterator iter = channelList.begin(); iter != channelList.end(); ++iter) {
			     		   std::string temp = *iter;
			     		   sprintf(channelsBuffer, "%s",temp.c_str());
			     		}
						if (sendto(mySocket, channelsBuffer, strlen(channelsBuffer), 0, (struct sockaddr *)&remoteAddress, remoteIPAddressSize)==-1)
							perror("server sending channel list error");
						std::cerr << "Success.\n";*/
					}
					else if (std::atoi(identifier.c_str()) == 5){//list of channels
						std::string channelString="";
						std::cerr << "channel list request: "<< channelString <<std::endl;	
			     		union intOrBytes channelListSize;
			     		channelListSize.integer = channelList.size();
						int thisBufSize = 4+4+32*channelListSize.integer;
						//std::cerr << "thisBufSize : " <<thisBufSize<<std::endl;
						
						char channelsBuffer[thisBufSize];//should be 4 + 4+32 * numchannels...? need to fix this list per spec
						for (int x=0; x<thisBufSize; x++){
		     		   		channelsBuffer[x]='\0';
		     		   	}   



						//sprintf(channelsBuffer, "%s","0001");
						std::cerr << "number of channels: " <<channelListSize.integer<<std::endl;
						std::string output = "";
						//int count = 0;
						//int pos = 8;
						union intOrBytes test;
						test.integer = channelListSize.integer;
						std::cerr << "test.integer = " << (int)test.integer<< std::endl;
						std::cerr << "test.byte[0] = " << (unsigned int)test.byte[0]<< std::endl;
						std::cerr << "test.byte[1] = " << (unsigned int)test.byte[1]<< std::endl;
						std::cerr << "test.byte[2] = " << (unsigned int)test.byte[2]<< std::endl;
						std::cerr << "test.byte[3] = " << (unsigned int)test.byte[3]<< std::endl;
						strcpy(channelsBuffer,"0001");
						//std::cerr << "channel list output to send: " <<channelsBuffer<<std::endl;
						
						std::string temp = "";
						//temp +=test.byte[0];
						//strcat(channelsBuffer,temp.c_str());
						channelsBuffer[4] = test.byte[0];
						channelsBuffer[5] = test.byte[1];
						channelsBuffer[6] = test.byte[2];
						channelsBuffer[7] = test.byte[3];
						std::cerr << "*****channelsBuffer[4] = " << (unsigned int)channelsBuffer[4]<< std::endl;
						std::cerr << "*****channelsBuffer[5] = " << (unsigned int)channelsBuffer[5]<< std::endl;
						std::cerr << "*****channelsBuffer[6] = " << (unsigned int)channelsBuffer[6]<< std::endl;
						std::cerr << "*****channelsBuffer[7] = " << (unsigned int)channelsBuffer[7]<< std::endl;

						/*
						//std::cerr << "channel list output to send: " <<channelsBuffer<<std::endl;
						

						temp = "";
						temp +=test.byte[1];
						strcat(channelsBuffer,temp.c_str());
						//std::cerr << "channel list output to send: " <<channelsBuffer<<std::endl;
						std::cerr << "*****channelsBuffer[5] = " << (unsigned int)channelsBuffer[4]<< std::endl;
						
						temp = "";
						temp +=test.byte[2];
						strcat(channelsBuffer,temp.c_str());
						//std::cerr << "channel list output to send: " <<channelsBuffer<<std::endl;
						
						temp = "";
						temp +=test.byte[3];
						strcat(channelsBuffer,temp.c_str());
						//std::cerr << "channel list output to send: " <<channelsBuffer<<std::endl;
						
*/

						/*temp +=test.byte[1];
						temp +=test.byte[2];
						temp +=test.byte[3];*/
						//std::cerr << "temp[0] = " << (unsigned int)temp[0]<< std::endl;
						
						//+test.byte[1]+test.byte[2]+test.byte[3];
						//std::string temp = (unsigned char)test.byte[0]+(unsigned char)test.byte[1]+(unsigned char)test.byte[2]+(unsigned char)test.byte[3];
			     		/*std::cerr<< "temp = "<<temp <<std::endl;
			     		//strcat(channelsBuffer,temp.c_str());
			     		std::cerr << "channel list output to send: " <<channelsBuffer<<std::endl;
						std::cerr << "size of channel list output to send: " <<sizeof(channelsBuffer)<<std::endl;*/
						int position = 8;
						
						 						
						for (std::vector<std::string>::iterator iter = channelList.begin(); iter != channelList.end(); ++iter) {
							uint x=0;		 
							std::string theCopy = *iter;
			     		    for (x=0; x<theCopy.length(); x++){//need error checking on input eventually, assuming it is <=32 here
			     		    	channelsBuffer[position+x] = theCopy[x];
			     		    }   
			     		    position+=32;
			     		
			     		}
			     		//if (sprintf(channelsBuffer, "0001%s%d",output.c_str(),test.integer)<0)
			     		//	perror("buffer overflow");
			     		/*std::cerr << "channel list output to send: " <<channelsBuffer<<std::endl;
						std::cerr << "size of channel list output to send: " <<sizeof(channelsBuffer)<<std::endl;*/
						if (sendto(mySocket, channelsBuffer, thisBufSize, 0, (struct sockaddr *)&remoteAddress, remoteIPAddressSize)==-1)
							perror("server sending channel list error");
						std::cerr << "Success.\n";
						//*//****need to format blocks for 32 bit*/
					}
					

					//show who is online
					for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
		     		   std::cerr << (*iter).myUserName << "is Online in channel "<< iter->myCurrentChannel << " at IP " << iter->myIPAddress<<std::endl;
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

