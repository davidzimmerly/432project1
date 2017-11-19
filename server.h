#include "shared.h"
//cis432 fall 2017 David Zimmerly
class server{
	public:
		std::string myIP;
		std::string myPort;
		server(char* domain, char* port);
		std::vector<serverInfo> serverList;
		void serve();
	private:
		struct sockaddr_in myAddress,remoteAddress;
		int bytesRecvd,mySocket;
		socklen_t addressSize;
		std::vector<channelInfo> mySubscribedChannels;

		std::vector<userInfo> currentUsers;
		int findUserSlot(std::string remoteIPAddress,int remotePort);
		void sendError(std::string theError, std::string ip, int port);
		void purgeUsers();
		void checkPurge(time_t &purgeTime);
		void sendMessage(std::string fromUser, /*int userPosition,*/ std::string toChannel, std::string message);
		void handleRequest(char* myBuffer,int bytesRecvd,std::string remoteIPAddress, int remotePort);
		void bindSocket();
		void leave(std::string userName, std::string channelToLeave);	
		bool amSubscribed(std::string channel){
			int result = findChannelInfoPositionInVector(mySubscribedChannels,channel);
			if (result!=-1)
				return true;
			else
				return false;
		}
		void sendS2Sjoin(std::string channel,std::string senderIP, std::string senderPort){//, char* buffer, bool useBuffer){
			std::cerr << myIP <<":"<<myPort<< " send sendS2Sjoin"<<std::endl;
			struct request_s2s_join* my_request_s2s_join= new request_s2s_join;
			initBuffer(my_request_s2s_join->req_channel, CHANNEL_MAX);
			my_request_s2s_join->req_type = REQ_S2S_JOIN;
			strcpy(my_request_s2s_join->req_channel,channel.c_str());



			for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
	 			if (!(((*iter).myIPAddress==senderIP)&&(*iter).myPort==std::atoi(senderPort.c_str()))){
					struct sockaddr_in remoteAddress;
					remoteAddress.sin_family = AF_INET;
					remoteAddress.sin_port = htons((*iter).myPort);
					remoteAddress.sin_addr.s_addr = inet_addr((*iter).myIPAddress.c_str());
					
					std::cerr << myIP <<":"<<myPort <<" "<<(*iter).myIPAddress<<":"<<(*iter).myPort<< " send S2S Join " <<channel<<std::endl;					

	 				if (sendto(mySocket, (char*)my_request_s2s_join, s2sJoinLeaveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
						perror("server sending s2s join request to multiple servers");
						exit(EXIT_FAILURE);
					}
				}
	 		}
			delete(my_request_s2s_join);

		}
		//almost same as above function, probably could combine
		void sendS2Sleave(std::string channel,std::string senderIP, std::string senderPort){//, char* buffer, bool useBuffer){
			std::cerr << myIP <<":"<<myPort<< " send sendS2Sleave"<<std::endl;
			struct request_s2s_leave* my_request_s2s_leave= new request_s2s_leave;
			initBuffer(my_request_s2s_leave->req_channel, CHANNEL_MAX);
			my_request_s2s_leave->req_type = REQ_S2S_LEAVE;
			strcpy(my_request_s2s_leave->req_channel,channel.c_str());
			


			for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
	 			if (!(((*iter).myIPAddress==senderIP)&&(*iter).myPort==std::atoi(senderPort.c_str()))){
					struct sockaddr_in remoteAddress;
					remoteAddress.sin_family = AF_INET;
					remoteAddress.sin_port = htons((*iter).myPort);
					remoteAddress.sin_addr.s_addr = inet_addr((*iter).myIPAddress.c_str());
					
					std::cerr << myIP <<":"<<myPort <<" "<<(*iter).myIPAddress<<":"<<(*iter).myPort<< " send S2S Leave " <<channel<<std::endl;					

	 				if (sendto(mySocket, (char*)my_request_s2s_leave, s2sJoinLeaveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
						perror("server sending s2s leave request to multiple servers");
						exit(EXIT_FAILURE);
					}
			}
	 		}
			delete(my_request_s2s_leave);

		}
		void sendS2Ssay(std::string fromUser, std::string toChannel,std::string message,std::string senderIP, int senderPort){
			struct request_s2s_say* my_request_s2s_say= new request_s2s_say;
			initBuffer(my_request_s2s_say->req_username, USERNAME_MAX);
			initBuffer(my_request_s2s_say->req_channel, CHANNEL_MAX);
			initBuffer(my_request_s2s_say->req_text, SAY_MAX);
			initBuffer(my_request_s2s_say->req_ID, ID_MAX);
			my_request_s2s_say->req_type = REQ_S2S_SAY;
			strcpy(my_request_s2s_say->req_channel,toChannel.c_str());
			strcpy(my_request_s2s_say->req_username,fromUser.c_str());
			strcpy(my_request_s2s_say->req_text,message.c_str());
			strcpy(my_request_s2s_say->req_ID,message.c_str());
			
/*			int randomData = open("/dev/random", O_RDONLY);
			if (randomData < 0)
			{
			    std::cerr << "error reading random data "<< std::endl;
			    exit(EXIT_FAILURE);
			}
			else
			{
			    char myRandomData[8];
			    size_t randomDataLen = 0;
			    while (randomDataLen < sizeof myRandomData)
			    {
			        ssize_t result = read(randomData, myRandomData + randomDataLen, (sizeof myRandomData) - randomDataLen);
			        if (result < 0)
			        {
			            std::cerr << "error reading random data "<< std::endl;
			    		exit(EXIT_FAILURE);
			        }
			        randomDataLen += result;
			    }
			    strcpy(my_request_s2s_say->req_ID,myRandomData);
			    close(randomData);
			}*/




			for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
				if (!(((*iter).myIPAddress==senderIP)&&(*iter).myPort==senderPort)){		
	 				struct sockaddr_in remoteAddress;
					remoteAddress.sin_family = AF_INET;
					remoteAddress.sin_port = htons((*iter).myPort);
					remoteAddress.sin_addr.s_addr = inet_addr((*iter).myIPAddress.c_str());
		



					std::cerr << myIP <<":"<<myPort <<" "<<(*iter).myIPAddress<<":"<<(*iter).myPort<< " send S2S Say " <<my_request_s2s_say->req_username<<" "<<my_request_s2s_say->req_channel<<" "<<"\""<<my_request_s2s_say->req_text<<"\""<<std::endl;					

	 				if (sendto(mySocket, (char*)my_request_s2s_say, s2sSaySize, 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
						perror("server sending s2s say request to multiple servers");
						exit(EXIT_FAILURE);
					}
				}
	 		}
			delete(my_request_s2s_say);

		}
		void say(std::string fromIP, int fromPort, std::string channel, std::string message){
			int userSlot = findUserSlot(fromIP,fromPort);
			if (userSlot>=0){//sending user found, proceed.
 				std::string userName=currentUsers[userSlot].myUserName;
     			std::cerr << fromIP <<":"<< fromPort <<" "<<myIP<<":"<<myPort<< " recv Request Say " <<userName<<" "<< std::endl;
     			sendMessage(userName/*, userSlot*/, channel, message);
				
				currentUsers[userSlot].lastSeen = time (NULL);
				//s2s say
			}

		}
		
};