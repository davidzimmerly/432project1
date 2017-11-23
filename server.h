#include "shared.h"
//cis432 fall 2017 David Zimmerly
class server{
	public:
		std::string myIP;
		std::string myPort;
		int myPortInt;
		server(char* domain, char* port);
		std::vector<serverInfo> serverList;
		void serve();
	private:
		struct sockaddr_in myAddress,remoteAddress;
		int bytesRecvd,mySocket;
		socklen_t addressSize;
		std::vector<channelInfo> mySubscribedChannels;
		std::vector<requestIDInfo> myRecentRequests;
		std::vector<userInfo> currentUsers;
		int findUserSlot(std::string remoteIPAddress,int remotePort);
		void sendError(std::string theError, std::string ip, int port);
		void purgeUsers();
		void checkPurge(time_t &purgeTime);
		void sendMessage(std::string fromUser, /*int userPosition,*/ std::string toChannel, std::string message);
		void handleRequest(char* myBuffer,int bytesRecvd,std::string remoteIPAddress, int remotePort);
		void bindSocket();
		void leave(std::string userName, std::string channelToLeave);	
		bool amSubscribed(std::string channel);
		void sendS2Sjoin(std::string channel,std::string senderIP, std::string senderPort);
		//almost same as above function, probably could combine
		void sendS2Sleave(std::string channel,std::string senderIP, std::string senderPort);
		void sendS2Ssay(std::string fromUser, std::string toChannel,std::string message,std::string senderIP, int senderPort,std::string fromIP, int fromPort, bool resend,char* buf);
		void handleSay(std::string fromIP, int fromPort, std::string channel, std::string message,char* buf);
		void handleS2SSay(std::string user, std::string channel, std::string message, std::string fromUser,std::string fromIP1, int fromPort1,std::string fromIP2, int fromPort2, char* buf);
		int findServerInfoPositionInVector(std::string ip, int port){
			int found = -1;
			for (unsigned int x=0; x<serverList.size(); x++) {
				if (serverList[x].myIPAddress==ip && serverList[x].myPort==port){
					found=x;
					break;
				}
			}
			return found;
		}
		int findID(char* input){
			int found = -1;
			for (unsigned int x=0; x<myRecentRequests.size(); x++) {
				if (strcmp(myRecentRequests[x].id,input)==0){
					found=x;
					break;
				}
			}
			return found;
		}
		bool isFromServer(std::string ip, int port){
			if (findServerInfoPositionInVector(ip,port)>-1)
				return true;
			else
				return false;
		}		

};