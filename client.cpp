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
#include <unistd.h>
#include <vector>
#include "shared.h"

#define BUFFERLENGTH  1024
#define THEPORT  3264





class client{
	private:
		int mySocket;
		struct sockaddr_in remoteAddress;
		struct sockaddr_in myAddress;
		socklen_t addressSize;
		std::string userName;
		std::string remoteAddressString;
		uint32_t bytesRecvd;
		
	public:
	void login(){
		std::cerr << "Logging In " << userName << " ..." ;
		unsigned char myBuffer[loginSize];
		//strcpy(myBuffer,"0000");
		myBuffer[0] = myBuffer[1] = myBuffer[2] = myBuffer[3] = '0';

		uint8_t choice = loginSize;
		if (choice>userName.length())
			choice = userName.length();
 		for (uint8_t x=0; x<choice; x++){//need error checking on input eventually, assuming it is <=32 here
		   	myBuffer[x+4] = userName[x];
		}   
		if (sendto(mySocket, myBuffer, loginSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		std::cerr << "Success.\n";
	}
	void logout(){
		std::cerr << "Logging Out " << userName << " ..." ;
		unsigned char myBuffer[logoutSize];
		//sprintf(myBuffer, "0001");
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '1';



		if (sendto(mySocket, myBuffer, logoutSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		std::cerr << "Success.\n";
		close(mySocket);
	}

	void requestChannels(){
		unsigned char myBuffer[requestChannelSize];
		unsigned char replyBuffer[BUFFERLENGTH];
		initBuffer(replyBuffer,BUFFERLENGTH);
		//sprintf(myBuffer, "0005");
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '5';

		if (sendto(mySocket, myBuffer, requestChannelSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		//printf("waiting for server reply on port %d\n", THEPORT);
		uint32_t bytesRecvd = recvfrom(mySocket, replyBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
		//printf("(channel request) client received %d bytes\n", bytesRecvd);
		if (bytesRecvd >= 36) {
			std::string identifier(&replyBuffer[0],&replyBuffer[4]);
			if (std::atoi(identifier.c_str()) == 1){//correct response code from server
				
				union intOrBytes totalChannels;
				totalChannels.byte[0] = replyBuffer[4];
				totalChannels.byte[1] = replyBuffer[5];
				totalChannels.byte[2] = replyBuffer[6];
				totalChannels.byte[3] = replyBuffer[7];
				std::vector<std::string> listOfChannels;
				uint8_t position=8;
				for (uint32_t x = 0; x <= totalChannels.integer; x++){
					std::string channel(&replyBuffer[position],&replyBuffer[position+32]);
					listOfChannels.push_back(channel);
					position+=32;
				}
				std::cerr << "List of received Channels: ";
				uint32_t count=1;
				for (std::vector<std::string>::iterator iter = listOfChannels.begin(); iter != listOfChannels.end(); ++iter) {
			    	std::cerr << *iter;
			    	if (count++<totalChannels.integer)
			    	 	std::cerr << ", ";

			    }
			    std::cerr<<std::endl;
			}
			
		}	
		
	}
	void join(std::string channel){
		if (channel.length()>31) //format input if too big
			channel = channel.substr(0,31);
		//std::cerr << "Joining channel " << channel << " ..." ;
		unsigned char myBuffer[joinSize];
		initBuffer(myBuffer,joinSize);
		uint8_t choice = joinSize;
		if (choice>channel.length())
			choice = channel.length();
		//strcpy(myBuffer,"0002");
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '2';

		for (uint8_t x=0; x<choice; x++){
		   	myBuffer[x+4] = channel[x];
		}   
		if (sendto(mySocket, myBuffer, joinSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("requesting to join a channel (from client)");
		//std::cerr << "Success.\n";
	}
	void leave(std::string channel){
		if (channel.length()>31) //format input if too big
			channel = channel.substr(0,31);

		std::cerr << "Leaving channel " << channel << " ..." ;
		unsigned char myBuffer[joinSize];
		initBuffer(myBuffer,joinSize);
		uint8_t choice = joinSize;
		if (choice>channel.length())
			choice = channel.length();
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '3';


		for (uint8_t x=0; x<choice; x++){
		   	myBuffer[x+4] = channel[x];
		}   
		if (sendto(mySocket, myBuffer, joinSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("requesting to leave a channel (from client)");
		std::cerr << "Success.\n";
	



	}	
	void who(std::string channel){
		std::cerr << "who is in channel " << channel << " ..." ;
		unsigned char myBuffer[joinSize];
		initBuffer(myBuffer,joinSize);
		uint8_t choice = joinSize;
		if (choice>channel.length())
			choice = channel.length();
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '6';


		for (uint8_t x=0; x<choice; x++){//need error checking on input eventually, assuming it is <=32 here
		   	myBuffer[x+4] = channel[x];
		}   
		if (sendto(mySocket, myBuffer, joinSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("requesting who in channel (from client)");
		std::cerr << "Success.\n";
	



	}


	client(){
		mySocket=0;
		//sockaddr_in remoteAddress;
		addressSize=0;
		userName="";		
		bytesRecvd=0;
		remoteAddressString="";
	}
	client(std::string user_name, std::string serverAddress){
		addressSize=sizeof(remoteAddress);
		userName=user_name;		
		bytesRecvd=0;
		remoteAddressString=serverAddress;
		mySocket=socket(AF_INET, SOCK_DGRAM, 0);
		if (mySocket==-1) {
			std::cerr << "socket created\n";
			
		}
		//char myBuffer[BUFFERLENGTH];
		memset((char *) &remoteAddress, 0, sizeof(remoteAddress));
		remoteAddress.sin_family = AF_INET;
		remoteAddress.sin_port = htons(THEPORT);
		if (inet_aton(remoteAddressString.c_str(), &remoteAddress.sin_addr)==0) {
			fprintf(stderr, "inet_aton() failed\n");
			exit(1);
		}

	}
};





int main (int argc, char *argv[]){
	client* thisClient = new client("Bobby Joeleine Smith4357093487509384750938475094387509348750439875430987435","127.0.0.1");
	thisClient->login();
	//thisClient->requestChannels();
	thisClient->join("newChannel");
	thisClient->join("newChannel");

	thisClient->leave("newChannel");
	
	//thisClient->join("anotherChannel");
	//thisClient->join("newChannel");
	thisClient->join("newChannelasfdkjh122342adskfj1111112");//ERRORS maybe client side
	thisClient->leave("newChannelasfdkjh122342adskfj1111112");//ERRORS maybe client side

	/*	thisClient->join("newChannelasfdkjhlkadf");
	thisClient->join("newChannel32432423423");
	thisClient->join("newChannel");
	thisClient->join("newChannel");
	thisClient->join("newChannel");
	thisClient->join("newChannel");*/

	//thisClient->requestChannels();
	thisClient->logout();
	
	
	
	delete(thisClient);
	return 0;

	
}

