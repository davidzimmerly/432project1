#include <stdio.h>
#include <stdlib.h>	/* needed for os x*/
#include <string.h>	/* strlen */
#include <netdb.h>      /* gethostbyname() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

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
			std::cerr << "Logging In " << userName << " ..." ;
			char myBuffer[BUFFERLENGTH];
			sprintf(myBuffer, "0000%s", userName.c_str());
			if (sendto(mySocket, myBuffer, strlen(myBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
				perror("sendto");
			std::cerr << "Success.\n";
			
	}
	void logout(){
			std::cerr << "Logging Out " << userName << " ..." ;
			char myBuffer[4];
			sprintf(myBuffer, "0001");
			if (sendto(mySocket, myBuffer, strlen(myBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
				perror("sendto");
			std::cerr << "Success.\n";
			close(mySocket);
	}

	void requestChannels(){
			char myBuffer[4];
			char replyBuffer[BUFFERLENGTH];
			for (int x=0;x<BUFFERLENGTH;x++){
				replyBuffer[x] ='\0';
			}
			sprintf(myBuffer, "0005");
			if (sendto(mySocket, myBuffer, strlen(myBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
				perror("sendto");
			std::cerr << "Success.\n";
			printf("waiting for server reply on port %d\n", THEPORT);
			//socklen_t remoteIPAddressSize = sizeof(remoteAddress);
			int bytesRecvd = recvfrom(mySocket, replyBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
			printf("received %d bytes\n", bytesRecvd);
			if (bytesRecvd >= 4) {
				std::string identifier(&replyBuffer[0],&replyBuffer[4]);//need to change to read until /r/l or something
				std::string remoteIPAddress=std::string (std::string (inet_ntoa(remoteAddress.sin_addr)));
				printf("***%s",replyBuffer);//need to format reply buffer
			}	
		
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
	thisClient->requestChannels();
	thisClient->logout();
	
	
	

	return 0;

	
}

