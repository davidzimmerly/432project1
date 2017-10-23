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
		//std::cerr << "Logging In " << userName << " ..." ;
		struct request_login* my_request_login= new request_login;
		my_request_login->req_type = REQ_LOGIN;
		strcpy(my_request_login->req_username,userName.c_str());
		if (sendto(mySocket, my_request_login, loginSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto login");
		delete(my_request_login);
		//std::cerr << "Success.\n";
	}
	void logout(){
		//std::cerr << "Logging Out " << userName << " ..." ;
		struct request_logout* my_request_logout= new request_logout;
		my_request_logout->req_type = REQ_LOGOUT;
		if (sendto(mySocket, my_request_logout, logoutSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto logout");
		close(mySocket);
		delete(my_request_logout);
	}

	void requestChannels(){
		 //std::cerr << "requesting channels" << std::endl;
		 //char myBuffer[requestChannelSize];
		 char replyBuffer[BUFFERLENGTH];
		//initBuffer(replyBuffer,BUFFERLENGTH);
		//sprintf(myBuffer, "0005");
		struct request_list* my_request_list= new request_list;
		my_request_list->req_type = REQ_LIST;
		if (sendto(mySocket, my_request_list, requestChannelSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("client requesting channels");
		//printf("waiting for server reply on port %d\n", THEPORT);
		bytesRecvd=0;
		int bytesRecvd = recvfrom(mySocket, replyBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
		//printf("(channel request) client received %d bytes\n", bytesRecvd);
		struct text_list* incoming_text_list;
		incoming_text_list = (struct text_list*)replyBuffer;
		//int channels = incoming_text_list->txt_nchannels;
		//std::cerr << "incoming got "<< channels << " channels."<<std::endl;
		


		if (/*incoming_text_list->txt_type==TXT_LIST && */ bytesRecvd>=32){
			int channels = incoming_text_list->txt_nchannels;
			std::cerr << "got "<< channels << " channels."<<std::endl;
		

			struct channel_info* txt_channels;
			txt_channels = (struct channel_info*)incoming_text_list->txt_channels;

			std::vector<std::string> listOfChannels;
			for (int x=0; x<channels; x++){
				listOfChannels.push_back(std::string(txt_channels[x].ch_channel));
			}
			
			std::cerr<<"Existing channels:"<<std::endl;
			for (std::vector<std::string>::iterator iter = listOfChannels.begin(); iter != listOfChannels.end(); ++iter) {
				std::cerr << " "<<*iter << std::endl;
			}
			

		}
		delete(my_request_list);

	}
	void say(std::string channel, std::string textfield){
		if (channel.length()>31) //format input if too big
			channel = channel.substr(0,31);
		if (textfield.length()>63) //format input if too big
			textfield = textfield.substr(0,63);
		

		//std::cerr << "Joining channel " << channel << " ..." ;
		 char myBuffer[sayRequestSize];
		initBuffer(myBuffer,sayRequestSize);
		unsigned int choice = joinLeaveSize;
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

		
		struct request_join* my_request_join= new request_join;
		my_request_join->req_type = REQ_JOIN;
		strcpy(my_request_join->req_channel,channel.c_str());
		if (sendto(mySocket, my_request_join, joinLeaveSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto request to join from client");
		delete(my_request_join);
		
	}
	void leave(std::string channel){
		if (channel.length()>31) //format input if too big
			channel = channel.substr(0,31);

		
		struct request_leave* my_request_leave= new request_leave;
		my_request_leave->req_type = REQ_LEAVE;
		strcpy(my_request_leave->req_channel,channel.c_str());
		if (sendto(mySocket, my_request_leave, joinLeaveSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto request to join from client");
		delete(my_request_leave);
		

	}	
	void who(std::string channel){
		std::cerr << "who is in channel " << channel << " ..." ;
		 char myBuffer[joinLeaveSize];
		initBuffer(myBuffer,joinLeaveSize);
		unsigned int choice = joinLeaveSize;
		if (choice>channel.length())
			choice = channel.length();
		myBuffer[0] = myBuffer[1] = myBuffer[2] = '0';
		myBuffer[3] = '6';


		for (unsigned int x=0; x<choice; x++){//need error checking on input eventually, assuming it is <=32 here
		   	myBuffer[x+4] = channel[x];
		}   
		if (sendto(mySocket, myBuffer, joinLeaveSize, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
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
	thisClient->join("Common");
//	thisClient->requestChannels();
	thisClient->join("newChannel");
	thisClient->leave("newChannel");
	/*thisClient->join("newChannel");

	
	
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

