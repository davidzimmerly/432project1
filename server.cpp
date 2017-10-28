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
		int findUserSlot(std::string remoteIPAddress){
			int userSlot=-1;
			int size=currentUsers.size();
			for (int x=0; x <size; x++){
				if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
					userSlot = x;
					break;
				}
			}
			return userSlot;		     			
		}

		void sendMessage(std::string fromUser, std::string toChannel, std::string message){
			//could probably recast the buffer instead of remaking it..
			int channelSlot = findChannelInfoPositionInVector(channelList,toChannel);
			if (channelSlot>-1){
				//get list of users/ips/ports, create text say, send message
				std::vector<std::string> listOfUsers = channelList[channelSlot].myUsers;
				std::vector<std::string> listOfIPs;
				std::vector<int> listOfPorts;
				int size = listOfUsers.size();
				for(int x =0 ; x < size; x++){
					//if (x!=userSlot){
						int position = findUserInfoPositionInVector(currentUsers,listOfUsers[x]);
						if (position>-1){
							listOfIPs.push_back(currentUsers[position].myIPAddress);
							listOfPorts.push_back(currentUsers[position].myPort);
						}
					//}
				}
				//create text_say
				struct text_say* my_text_say= new text_say;
				my_text_say->txt_type = TXT_SAY;
				strcpy(my_text_say->txt_channel,toChannel.c_str());
				strcpy(my_text_say->txt_text,message.c_str());
				strcpy(my_text_say->txt_username,fromUser.c_str());
				if (listOfUsers.size()==listOfIPs.size()&&listOfIPs.size()==listOfPorts.size()){
					for(unsigned int x =0 ; x < listOfUsers.size(); x++){
						struct sockaddr_in remoteAddress;
						remoteAddress.sin_family = AF_INET;
						remoteAddress.sin_port = htons(listOfPorts[x]);
						remoteAddress.sin_addr.s_addr = inet_addr(listOfIPs[x].c_str());\
						std::cerr<< "sending mail to "<<listOfUsers[x]<<" at ip "<<listOfIPs[x]<< " on port "<< listOfPorts[x]<<std::endl;
						if (sendto(mySocket, (char*)my_text_say, saySize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
							perror("server sending mail to multiple users");
							exit(-1);
						}
					}
					
				}
				delete(my_text_say);
			}
		}

		void handleRequest(char* myBuffer,int bytesRecvd){
			if (bytesRecvd >= logoutListSize) {
				struct request* incoming_request = new request;
				incoming_request = (struct request*)myBuffer;
				request_t identifier = incoming_request->req_type;
				std::string remoteIPAddress=inet_ntoa(remoteAddress.sin_addr);
				int remotePort =htons(remoteAddress.sin_port);
				socklen_t remoteIPAddressSize = sizeof(remoteAddress);
				if (identifier == REQ_LOGIN && bytesRecvd==loginSize){//login
					if (bytesRecvd==loginSize){//checks valid login packet size
						struct request_login* incoming_login_request;
						incoming_login_request = (struct request_login*)myBuffer;
						std::string userName = std::string(incoming_login_request->req_username);
						std::cerr << "server: " << userName <<" logs in" << std::endl;
						bool addUser = true;
						if (currentUsers.size()>0) {
							for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
			     				if (userName.compare((*iter).myUserName) == 0){
			     					std::cerr<< " error , user is already logged in.";//error message? see what binary does
			     					addUser = false;
			     				}
			     			}
			     		}
			     		if (addUser){
			     			userInfo newUser;
			     			newUser.myUserName = userName;
			     			newUser.myIPAddress = remoteIPAddress;
			     			newUser.myPort = remotePort;
			     			newUser.myChannels.push_back("Common");
			     			newUser.myActiveChannel = "Common";
			     			currentUsers.push_back(newUser);
			     		}
			     	}
				}
				else if (identifier == REQ_LOGOUT && bytesRecvd==logoutListSize){//logout
					if (currentUsers.size()>0) {
						int size =  currentUsers.size();
						for (int x=0; x <size; x++){
							if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0){
		     					
		     					std::cerr << "server: " << currentUsers[x].myUserName <<" logs out" << std::endl;
		     					currentUsers.erase(currentUsers.begin()+x);
		     					break;

		     				}
		     			}
		     		}
				}
				else if (identifier == REQ_JOIN && bytesRecvd == joinLeaveWhoSize){//join request
					struct request_join* incoming_join_request;
					incoming_join_request = (struct request_join*)myBuffer;
					std::string channelToJoin = std::string(incoming_join_request->req_channel);
					int userSlot = findUserSlot(remoteIPAddress);

	     			if (userSlot>=0){//user found in macro collection
	     				std::string userName = currentUsers[userSlot].myUserName;
     					bool channelFound=false;
     					int position =-1;
						for (unsigned int x=0; x <channelList.size(); x++){
							if (channelToJoin.compare(channelList[x].myChannelName) == 0){
		     					channelFound=true;
		     					position = x;
		     					break;
		     				}
						}
		     			if (!channelFound){//if channel not exist, add it
		     				struct channelInfo newChannel;
		     				newChannel.myChannelName = channelToJoin;
		     				newChannel.myUsers.push_back(userName);
		     				channelList.push_back(newChannel);
		     			}
		     			else{//channel was found
		     				bool userFound=false;//check if user exists in channel already
							for (unsigned int x=0; x <channelList[position].myUsers.size(); x++){
								if (userName.compare(channelList[position].myUsers[x]) == 0){
			     					userFound=true;
			     					break;
			     				}
							}
							if (!userFound){
								channelList[position].myUsers.push_back(userName);
								std::cerr << "server: " << userName <<" joins channel " << channelToJoin <<std::endl;
							}
	     				}
     					currentUsers[userSlot].myActiveChannel = channelToJoin;
     					
	     			
	     			
		     		}
				}
				else if (identifier == REQ_LEAVE && bytesRecvd == joinLeaveWhoSize){//leave request
					struct request_leave* incoming_leave_request;
					incoming_leave_request = (struct request_leave*)myBuffer;
					std::string channelToLeave = std::string(incoming_leave_request->req_channel);
					int userSlot = findUserSlot(remoteIPAddress);
					std::string userName = currentUsers[userSlot].myUserName;
	     			if (userSlot>=0){//user found, erase from inside channel list
	     				for (unsigned int x=0; x <currentUsers[userSlot].myChannels.size(); x++){
							if (channelToLeave.compare(currentUsers[userSlot].myChannels[x]) == 0){
		     					currentUsers[userSlot].myChannels.erase(currentUsers[userSlot].myChannels.begin() + x);
		     					break;
		     				}
		     			}
	     				currentUsers[userSlot].myActiveChannel = "";
	     				std::cerr << "server: " << userName <<" leaves channel " << channelToLeave <<std::endl;

	     				int masterChannelListPosition =-1;
	     				int channelListNamePosition =-1;
	     				for (unsigned int x=0; x <channelList.size(); x++){
							if (channelToLeave.compare(channelList[x].myChannelName) == 0){
		     					masterChannelListPosition=x;
		     					break;
		     				}
		     			}
	     				if (masterChannelListPosition>-1){
		     				for (unsigned int x=0; x <channelList[masterChannelListPosition].myUsers.size(); x++){
								if (userName.compare(channelList[masterChannelListPosition].myUsers[x]) == 0){
			     					channelListNamePosition=x;
			     					break;
			     				}
			     			}
			     		}
		     			if (channelListNamePosition>-1){
		     				channelList[masterChannelListPosition].myUsers.erase(channelList[masterChannelListPosition].myUsers.begin()+channelListNamePosition);
							if (channelList[masterChannelListPosition].myUsers.size()==0){
								channelList.erase(channelList.begin()+masterChannelListPosition);
								std::cerr << "server: removing empty channel "<<channelToLeave<<std::endl;
							}
		     			}
	     				
	     			}
				}
				else if (identifier == REQ_SAY && bytesRecvd==sayRequestSize){//say request
					//std::cerr << "detected say request "<< std::endl;
		     			
					struct request_say* incoming_request_say;
					incoming_request_say = (struct request_say*)myBuffer;
					std::string channelToAnnounce = std::string(incoming_request_say->req_channel);
					std::string textField = std::string(incoming_request_say->req_text);						
					int userSlot = findUserSlot(remoteIPAddress);
					
	     			if (userSlot>=0){//user found
	     				std::string userName=currentUsers[userSlot].myUserName;
	     				//std::cerr << "say request received from "<< "userName: " <<userName << "for channel " << channelToAnnounce << std::endl;
		     			//std::cerr <<"msg: "<< textField << std::endl;
		     			//std::cerr <<"about to run sendMessage "<< std::endl;
		     			
		     			sendMessage(userName, channelToAnnounce, textField);

					}
				}
				else if (identifier == REQ_SWITCH && bytesRecvd==logoutListSize){//say request
					struct request_switch* incoming_request_switch;
					incoming_request_switch = (struct request_switch*)myBuffer;
					std::string channel = std::string(incoming_request_switch->req_channel);
					
					int userSlot = findUserSlot(remoteIPAddress);
					
	     			if (userSlot>=0){//user found
	     			//now want to check if user is subscribed to channel
	     				if (findStringPositionInVector(currentUsers[userSlot].myChannels,channel)>-1)
	     					currentUsers[userSlot].myActiveChannel = channel;
					}//need to send error message back to client : You have not subscribed to channel channel******
				}
				else if (identifier == REQ_LIST && bytesRecvd==logoutListSize){//list of channels
					int size = channelList.size();
					int reserveSize = sizeof(text_list)+sizeof(channel_info)*size-1;
					struct text_list* my_text_list = (text_list*)malloc(reserveSize);
					my_text_list->txt_type = TXT_LIST;
					my_text_list->txt_nchannels = channelList.size();
					for (unsigned int x=0; x<channelList.size(); x++){
						strcpy(my_text_list->txt_channels[x].ch_channel,channelList[x].myChannelName.c_str());
					}
					if (sendto(mySocket, my_text_list, reserveSize, 0, (struct sockaddr *)&remoteAddress, remoteIPAddressSize)==-1){
						perror("server sending channel list error");
						exit(-1);
					}
					free(my_text_list);
				}
				else if (identifier == REQ_WHO && bytesRecvd==joinLeaveWhoSize){//list of people on certain channel
					struct request_who* incoming_request_who;
					incoming_request_who = (struct request_who*)myBuffer;
					std::string channelToQuery = std::string(incoming_request_who->req_channel);
					int userSlot = findUserSlot(remoteIPAddress);
					std::string userName = currentUsers[userSlot].myUserName;
	     			

					//find given channel, check size
					int position =-1;
					for (unsigned int x=0; x<channelList.size(); x++){
						if (channelToQuery.compare(channelList[x].myChannelName)==0){
							position=x;
							break;
						}
					}
					if (position>-1){
						int size = channelList[position].myUsers.size();
						int reserveSize = sizeof(text_who)+sizeof(user_info)*size-1;
						struct text_who* my_text_who = (text_who*)malloc(reserveSize);
						my_text_who->txt_type = TXT_WHO;
						my_text_who->txt_nusernames = size;
						strcpy(my_text_who->txt_channel,channelToQuery.c_str());
						for (unsigned int x=0; x<channelList[position].myUsers.size(); x++){
							strcpy(my_text_who->txt_users[x].us_username,channelList[position].myUsers[x].c_str());
						}
						if (sendto(mySocket, my_text_who, reserveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
							perror("server sending who is error");
							exit(-1);
						}
					}
					
				}
					

			}

		}

		void serve(){
			while(true) {
				int bytesRecvd = 0;
				char myBuffer[BUFFERLENGTH];
				std::cerr <<"bound IP address:" << myDomain << " waiting on Port: "<< myPort <<std::endl;
				//printf("waiting on port %d\n", port.c_str);
				bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
				
				printf("received %d bytes\n", bytesRecvd);
				handleRequest(myBuffer,bytesRecvd);				
			}
		}
		
	public:
		std::string myDomain,myPort;
		server(char* domain, char* port){
			myDomain = std::string(domain);
			myPort = std::string(port);
			addressSize = sizeof(myAddress);
			bytesRecvd=0;
			mySocket=socket(AF_INET, SOCK_DGRAM, 0);
			if (mySocket<0) {
				perror ("socket() failed");
				exit(-1);
			}
			struct channelInfo newChannel;
			newChannel.myChannelName = "Common";
			channelList.push_back(newChannel);
			myAddress.sin_family = AF_INET;
			myAddress.sin_addr.s_addr = inet_addr(domain);//htonl(INADDR_ANY);//inet_addr("128.223.4.39");//
			myAddress.sin_port = htons(std::atoi(port));
			if (bind(mySocket, (struct sockaddr *)&myAddress, sizeof(myAddress)) < 0) {
				perror("bind failed");
				exit(-1);
			}
			serve();
		}
};

int main (int argc, char *argv[]){
	if (argc!=3){
		std::cerr<<"Usage: ./server domain_name port_num"<<std::endl;
		exit(EXIT_FAILURE);
	}
	

	server* myServer = new server(argv[1],argv[2]);
	delete(myServer);
	return 0;
}