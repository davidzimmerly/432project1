#include "shared.h"

class client{
	public:
		int mySocket;
		client();
		client(char* serverAddress, char* serverPort, char* userName);
		int getServerResponse(bool nonblocking, char* replyBuffer);
		void handleServerResponse(char* replyBuffer,int bytesRecvd);
		void login();
		void logout();
		void join(std::string channel);
		void keepAlive();
		bool parseCommand(std::string buffer);		
	private:
		
		struct sockaddr_in remoteAddress, myAddress;
		socklen_t addressSize;
		std::string myUserName, remoteAddressString, myActiveChannel;
		int remotePort, bytesRecvd;
		std::vector<std::string> myChannels;
		void leave(std::string channel);
		void requestChannels();
		void say(std::string textfield);
		void send(char* buf,int size,std::string error);
		void switchChannel(std::string channel);
		void who(std::string channel);
		
};

int main (int argc, char *argv[]){
	if (argc!=4){
		std::cerr<<"Usage: ./client server_socket server_port username"<<std::endl;
		exit(EXIT_FAILURE);
	}
	
	client* thisClient = new client(argv[1],argv[2],argv[3]);
	thisClient->login();
	thisClient->join("Common");
	bool running = true;
	std::string command="";
	raw_mode();
	std::string buffer="";
	if (buffer==""){
			std::cerr<<">";//<<buffer;		
		}
	/*time_t now;
    time(&now);
 
    struct tm beg;
    beg = *localtime(&now);	*/
	struct timeval* timeOut=new timeval;
	while (running){
		timeOut->tv_sec = 60;
		timeOut->tv_usec = 0;	
		
				


		char replyBuffer[BUFFERLENGTH];
		int err;
		fd_set readfds;
		FD_ZERO (&readfds);
		FD_SET (thisClient->mySocket, &readfds);
		FD_SET (STDIN_FILENO, &readfds);
		err = select (thisClient->mySocket + 1, &readfds, NULL, NULL, timeOut);
		if (err < 0) perror ("select failed");
		else if (err==0){
			//time(&now);
			//double seconds = difftime(now, mktime(&beg));
			//if (seconds>.0)
			std::cerr<<"keep alive sent "<<std::endl;
			thisClient->keepAlive();
			timeOut->tv_sec = 60;//err=1;
		}
		else {
	        if (FD_ISSET (thisClient->mySocket, &readfds)){
	        	int bytesRecvd = thisClient->getServerResponse(false,replyBuffer);
				if (bytesRecvd>0){
					for (unsigned int x=0; x <= buffer.size(); x++){
						std::cerr<<'\b';
					}
					thisClient->handleServerResponse(replyBuffer,bytesRecvd);
					std::cerr<<'>'<<buffer;
				}
	        }
	        if (FD_ISSET (STDIN_FILENO, &readfds)){
	        	char c;
	        	c = fgetc(stdin);
	        	if (c=='\n'){//run the command
	        		std::cerr<<std::endl;
	        		running = thisClient->parseCommand(buffer);
	        		std::cerr<<'\b'<<'\b'<<'>';
	        		buffer="";
	        	}
	        	else if (c==127){//backspace
	        		if (buffer.length()>0){
	        			std::cerr<<'\b'<<' '<<'\b';
						buffer=buffer.substr(0,buffer.length()-1);
					}
	        	}
	        	else {//output a character
	        		buffer += c;
	        		std::cerr<<c;
	        	}
				FD_CLR(STDIN_FILENO,&readfds);
	        }

		}
    	
	}
	thisClient->logout();
	delete(thisClient);
	delete(timeOut);
	return 0;
}