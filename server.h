#include "shared.h"

class server{
	private:
		struct sockaddr_in myAddress;
		struct sockaddr_in remoteAddress;
		int bytesRecvd;
		socklen_t addressSize;
		int mySocket;
		std::vector<userInfo> currentUsers;
		std::vector<channelInfo> channelList;
		int findUserSlot(std::string remoteIPAddress,int remotePort);
		void sendError(std::string theError, std::string ip, int port);
		void purgeUsers();
		void sendMessage(std::string fromUser, std::string toChannel, std::string message);
		void handleRequest(char* myBuffer,int bytesRecvd);
		void serve();
	public:
		std::string myDomain,myPort;
		server(char* domain, char* port);
};

