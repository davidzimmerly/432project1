#include "shared.h"
class client{
	public:
		int mySocket;
		client();
		client(char* serverAddress, char* serverPort, char* userName);
		bool parseCommand(std::string buffer);		
		int getServerResponse(bool nonblocking, char* replyBuffer);
		void checkKeepAlive(time_t &keepAliveTime);
		void handleServerResponse(char* replyBuffer,int bytesRecvd);
		void login();
		void logout();
		void join(std::string channel);
		void keepAlive();		
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