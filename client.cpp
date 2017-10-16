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




void login(int socket,sockaddr_in remoteAddress,int addressSize,std::string userName){
		std::cerr << "Logging In " << userName << " ..." ;
		char myBuffer[BUFFERLENGTH];
		sprintf(myBuffer, "0000%s", userName.c_str());
		if (sendto(socket, myBuffer, strlen(myBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		std::cerr << "Success.\n";
		
}
void logout(int socket,sockaddr_in remoteAddress,int addressSize){
		//std::cerr << "Logging Out " << userName << " ..." ;
		char myBuffer[4];
		sprintf(myBuffer, "0001");
		if (sendto(socket, myBuffer, strlen(myBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		std::cerr << "Success.\n";
		
}

void requestChannels(int socket,sockaddr_in remoteAddress,int addressSize){
		char myBuffer[4];
		char replyBuffer[BUFFERLENGTH];
		for (int x=0;x<BUFFERLENGTH;x++){
			replyBuffer[x] ='\0';
		}
		sprintf(myBuffer, "0005");
		if (sendto(socket, myBuffer, strlen(myBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		std::cerr << "Success.\n";
		printf("waiting for server reply on port %d\n", THEPORT);
		socklen_t remoteIPAddressSize = sizeof(remoteAddress);
		int bytesRecvd = recvfrom(socket, replyBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &remoteIPAddressSize);
		printf("received %d bytes\n", bytesRecvd);
		if (bytesRecvd >= 4) {
			std::string identifier(&replyBuffer[0],&replyBuffer[4]);//need to change to read until /r/l or something
			std::string remoteIPAddress=std::string (std::string (inet_ntoa(remoteAddress.sin_addr)));
			printf("***%s",replyBuffer);
		}	
	
}


int main (int argc, char *argv[]){
	std::string userName = "Bobby Joeleine Smith4357093487509384750938475094387509348750439875430987435";
	struct sockaddr_in myAddress;
	struct sockaddr_in remoteAddress;
	socklen_t addressSize = sizeof(remoteAddress);
	int bytesRecvd;
	int mySocket;
	std::string serverAddress = "127.0.0.1";//"128.223.4.39";
	int MSGS = 1;

	mySocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (mySocket==-1) {
		std::cerr << "socket created\n";
		
	}
	char myBuffer[BUFFERLENGTH];
	memset((char *) &remoteAddress, 0, sizeof(remoteAddress));
	remoteAddress.sin_family = AF_INET;
	remoteAddress.sin_port = htons(THEPORT);
	if (inet_aton(serverAddress.c_str(), &remoteAddress.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	login(mySocket,remoteAddress,addressSize,"billy joe234234234234243243432243432243432432432");
	requestChannels(mySocket,remoteAddress,addressSize);
	logout(mySocket,remoteAddress,addressSize);

	close(mySocket);
	

	return 0;

	
}

