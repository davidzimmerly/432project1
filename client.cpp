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
		int bytesRecvd;
		
	public:
	void login(){
		if (userName.length()>31)//truncate username if too big
			userName = userName.substr(0,31);
		std::cerr << "Logging In " << userName << " ..." ;
		struct request_login* my_request_login= new request_login;
		my_request_login->req_type = REQ_LOGIN;
		strcpy(my_request_login->req_username,userName.c_str());
		if (sendto(mySocket, my_request_login, loginSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto login");
		std::cerr << "Success.\n";
	}
	void logout(){
		std::cerr << "Logging Out " << userName << " ..." ;
		struct request_logout* my_request_logout= new request_logout;
		my_request_logout->req_type = REQ_LOGOUT;
		if (sendto(mySocket, my_request_logout, logoutSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto logout");
		close(mySocket);
		std::cerr << "Success.\n";
	}

	void requestChannels(){
		 char myBuffer[requestChannelSize];
		 char replyBuffer[BUFFERLENGTH];
		initBuffer(replyBuffer,BUFFERLENGTH);
		//sprintf(myBuffer, "0005");
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '5';

		if (sendto(mySocket, myBuffer, requestChannelSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		//printf("waiting for server reply on port %d\n", THEPORT);
		int bytesRecvd = recvfrom(mySocket, replyBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
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
				int position=8;
				for (int x = 0; x <= totalChannels.integer; x++){
					std::string channel(&replyBuffer[position],&replyBuffer[position+32]);
					listOfChannels.push_back(channel);
					position+=32;
				}
				std::cerr << "List of received Channels: ";
				int count=1;
				for (std::vector<std::string>::iterator iter = listOfChannels.begin(); iter != listOfChannels.end(); ++iter) {
			    	std::cerr << *iter;
			    	if (count++<totalChannels.integer)
			    	 	std::cerr << ", ";

			    }
			    std::cerr<<std::endl;
			}
			
		}	
		
	}
	void say(std::string channel, std::string textfield){
		if (channel.length()>31) //format input if too big
			channel = channel.substr(0,31);
		if (textfield.length()>63) //format input if too big
			textfield = textfield.substr(0,63);
		

		//std::cerr << "Joining channel " << channel << " ..." ;
		 char myBuffer[sayRequestSize];
		initBuffer(myBuffer,sayRequestSize);
		unsigned int choice = joinSize;
		if (choice>channel.length())
			choice = channel.length();
		//strcpy(myBuffer,"0004");
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '4';

		for (unsigned int x=0; x<choice; x++){
		   	myBuffer[x+4] = channel[x];
		}   
		//now the textfield
		choice = sayRequestSize;
		if (choice>textfield.length())
			choice = textfield.length();
		for (unsigned int x=0; x<choice; x++){
		   	myBuffer[x+36] = textfield[x];
		}   
		


		if (sendto(mySocket, myBuffer, sayRequestSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sending a message to channel  (from client)");
		//std::cerr << "Success.\n";
	}
	
	void join(std::string channel){
		if (channel.length()>31) //format input if too big
			channel = channel.substr(0,31);
		//std::cerr << "Joining channel " << channel << " ..." ;
		char myBuffer[joinSize];
		initBuffer(myBuffer,joinSize);
		unsigned int choice = joinSize;
		if (choice>channel.length())
			choice = channel.length();
		//strcpy(myBuffer,"0002");
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '2';

		for (unsigned int x=0; x<choice; x++){
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
		 char myBuffer[joinSize];
		initBuffer(myBuffer,joinSize);
		unsigned int choice = joinSize;
		if (choice>channel.length())
			choice = channel.length();
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '3';


		for (unsigned int x=0; x<choice; x++){
		   	myBuffer[x+4] = channel[x];
		}   
		if (sendto(mySocket, myBuffer, joinSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("requesting to leave a channel (from client)");
		std::cerr << "Success.\n";
	



	}	
	void who(std::string channel){
		std::cerr << "who is in channel " << channel << " ..." ;
		 char myBuffer[joinSize];
		initBuffer(myBuffer,joinSize);
		unsigned int choice = joinSize;
		if (choice>channel.length())
			choice = channel.length();
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '6';


		for (unsigned int x=0; x<choice; x++){//need error checking on input eventually, assuming it is <=32 here
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
/*	thisClient->join("newChannel");
	thisClient->join("newChannel");

	thisClient->leave("newChannel");
	
	//thisClient->join("anotherChannel");
	//thisClient->join("newChannel");
	thisClient->join("newChannelasfdkjh122342adskfj1111112");//ERRORS maybe client side
	thisClient->leave("newChannelasfdkjh122342adskfj1111112");//ERRORS maybe client side
*/
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

