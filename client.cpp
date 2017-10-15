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
void logout(int socket,sockaddr_in remoteAddress,int addressSize,std::string userName){
		std::cerr << "Logging Out " << userName << " ..." ;
		char myBuffer[BUFFERLENGTH];
		sprintf(myBuffer, "0001%s", userName.c_str());
		if (sendto(socket, myBuffer, strlen(myBuffer), 0, (struct sockaddr *)&remoteAddress, addressSize)==-1)
			perror("sendto");
		std::cerr << "Success.\n";
		
}


int main (int argc, char *argv[]){
	std::string userName = "Bobby Joeleine Smith4357093487509384750938475094387509348750439875430987435";
	struct sockaddr_in myAddress;
	struct sockaddr_in remoteAddress;
	socklen_t addressSize = sizeof(remoteAddress);
	int bytesRecvd;
	int mySocket;
	std::string serverAddress = "127.0.0.1";
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
	close(mySocket);
	

	return 0;

	
}

