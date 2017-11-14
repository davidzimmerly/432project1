#include "shared.h"
//cis432 fall 2017 David Zimmerly
class server{
	public:
		std::string myDomain,myPort;
		server(char* domain, char* port);
		std::vector<serverInfo> serverList;
		void serve();
	private:
		struct sockaddr_in myAddress,remoteAddress;
		int bytesRecvd,mySocket;
		socklen_t addressSize;
		std::vector<channelInfo> channelList;

		std::vector<userInfo> currentUsers;
		int findUserSlot(std::string remoteIPAddress,int remotePort);
		void sendError(std::string theError, std::string ip, int port);
		void purgeUsers();
		void checkPurge(time_t &purgeTime);
		void sendMessage(std::string fromUser, std::string toChannel, std::string message);
		void handleRequest(char* myBuffer,int bytesRecvd);
		
		void leave(std::string userName, std::string channelToLeave);	
};