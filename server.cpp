#include "server.h"
//cis432 fall 2017 David Zimmerly project 2
int main (int argc, char *argv[]){
	if (argc<3 || argc%2==0){
		std::cerr<<"Usage: ./server domain_name port_num neighbor_server_1_IP neighbor_server_1_Port... neighbor_serverb_X_IP neighbor_serverb_X_Port"<<std::endl;
		exit(EXIT_FAILURE);
	}
	int count = 5;//if >=5 (not even) parameters, start adding adjacent servers
	server* myServer = new server(argv[1],argv[2]);
	while (argc>=count){
		struct serverInfo newNeighbor;
		struct hostent     *he;
		newNeighbor.myAddress.sin_family = AF_INET;
		newNeighbor.myAddress.sin_port = htons(std::atoi(argv[count-1]));
		if ((he = gethostbyname(argv[count-2])) == NULL) {
			puts("error resolving hostname..");
			exit(1);
		}
		memcpy(&newNeighbor.myAddress.sin_addr, he->h_addr_list[0], he->h_length);
		newNeighbor.myIPAddress=inet_ntoa(newNeighbor.myAddress.sin_addr);//replace localhost to 127.0.0.1 internally as in docs
		newNeighbor.myPort =  std::atoi(argv[count-1]);
		myServer->serverList.push_back(newNeighbor);
		count +=2; 
	}
	myServer->serve();
	delete(myServer);
	return 0;
}
int server::findUserSlot(std::string remoteIPAddress,int remotePort){
	int userSlot=-1;
	int size=currentUsers.size();
	for (int x=0; x <size; x++){
		if (remoteIPAddress.compare(currentUsers[x].myIPAddress) == 0 && remotePort==currentUsers[x].myPort){
			userSlot = x;
			break;
		}
	}
	return userSlot;		     			
}
void server::sendError(std::string theError, std::string ip, int port){
	//create error message
	struct text_error* my_text_error = new text_error;
	my_text_error->txt_type = TXT_ERROR;
	initBuffer(my_text_error->txt_error, SAY_MAX);
	strcpy(my_text_error->txt_error,theError.c_str());
	struct sockaddr_in remoteAddress;
	remoteAddress.sin_family = AF_INET;
	remoteAddress.sin_port = htons(port);
	remoteAddress.sin_addr.s_addr = inet_addr(ip.c_str());\
	std::cerr<< " sending error "<<theError<<"to user at ip "<<ip<< " on port "<< port<<std::endl;
	if (sendto(mySocket, (char*)my_text_error, saySize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
		perror("server sending error to  user");
		exit(EXIT_FAILURE);
	}
	delete(my_text_error);
}
void server::purge(){
	time_t timeNow = time(NULL);
	for (unsigned int x=0; x<currentUsers.size(); x++){
		double seconds = difftime(timeNow,currentUsers[x].lastSeen);
		if (seconds>=serverTimeout){
			//log out of user subscribed channels:
			for (unsigned int y=0; y<currentUsers[x].myChannels.size(); y++){
				server::leave(currentUsers[x].myUserName,currentUsers[x].myChannels[y]);
			}
			//log out of server subscribed channels:
			for (unsigned int y=0; y<mySubscribedChannels.size(); y++){
				server::leave(currentUsers[x].myUserName,mySubscribedChannels[y].myChannelName);
			}
			std::cerr << myIP <<":"<<myPort <<" "<<currentUsers[x].myIPAddress<<":"<<currentUsers[x].myPort<< " purge log out " <<currentUsers[x].myUserName<<std::endl;					
	 		if (currentUsers.size()==1){
     			currentUsers.erase(currentUsers.begin());
     			break;
     		}
     		else{
     			currentUsers.erase(currentUsers.begin()+x);
         		if (x+1<=currentUsers.size())//at least one more?  this deserves more testing, but seems to work
	     			x--;
	     		else
	     			break;	
	     	}					
		}
	}
	for (unsigned int x=0; x<serverList.size(); x++){//purge channels of adj servers if have not resent join message
		std::vector<int> removeThese;
		for (unsigned int y=0; y<serverList[x].myChannels.size(); y++){
			double seconds = difftime(timeNow,serverList[x].myTimeStamps[y]);
			if (seconds>=serverTimeout){
				removeThese.push_back(y);
			}
		}
		for (unsigned int z=removeThese.size()-1; z<=0; z--){
			serverList[x].myChannels.erase(serverList[x].myChannels.begin()+removeThese[z]);
			serverList[x].myTimeStamps.erase(serverList[x].myTimeStamps.begin()+removeThese[z]);
			std::cerr << myIP <<":"<<myPort <<" "<<serverList[x].myIPAddress<<":"<<serverList[x].myPort<< " purge log out server "<<std::endl;					
	 		
		}
	}
}
void server::sendMessage(std::string fromUser, std::string toChannel, std::string message){
	int channelSlot = findChannelInfoPositionInVector(mySubscribedChannels,toChannel);
	if (channelSlot>-1){
		//get list of users/ips/ports, create text say, send message
		std::vector<struct userInfo> listOfUsers = mySubscribedChannels[channelSlot].myUsers;
		struct text_say* my_text_say= new text_say;
		my_text_say->txt_type = TXT_SAY;
		initBuffer(my_text_say->txt_channel, CHANNEL_MAX);
		initBuffer(my_text_say->txt_text, SAY_MAX);
		initBuffer(my_text_say->txt_username, USERNAME_MAX);
		strcpy(my_text_say->txt_channel,toChannel.c_str());
		strcpy(my_text_say->txt_text,message.c_str());
		strcpy(my_text_say->txt_username,fromUser.c_str());
		bool sent=false;
		for(unsigned int x =0 ; x < listOfUsers.size(); x++){
			struct sockaddr_in remoteAddress;
			remoteAddress.sin_family = AF_INET;
			remoteAddress.sin_port = htons(listOfUsers[x].myPort);
			remoteAddress.sin_addr.s_addr = inet_addr(listOfUsers[x].myIPAddress.c_str());\
			//std::cerr<< " sending mail to "<<listOfUsers[x].myUserName<<" at ip "<<listOfUsers[x].myIPAddress<< " on port "<< listOfUsers[x].myPort<<std::endl;
			if (sendto(mySocket, (char*)my_text_say, saySize, 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
				perror("server sending mail to multiple users");
				exit(EXIT_FAILURE);
			}
			else
				sent=true;
		}
		delete(my_text_say);
		if (!sent){
			std::cerr << " list of users is empty trying to sendmail"<< std::endl;	
			exit(EXIT_FAILURE);
		}
	}
	else{
		std::cerr << "channel not found on server trying to sendmail"<< std::endl;
		exit(EXIT_FAILURE);
	}
}
void server::handleRequest(char* myBuffer,int bytesRecvd,std::string remoteIPAddress, int remotePort){
	if (bytesRecvd>=BUFFERLENGTH){
		std::cerr << "*buffer overflow, ignoring request" << std::endl;
		sendError("*buffer overflow error",remoteIPAddress,remotePort);
	}
	else if (bytesRecvd >= logoutListKeepAliveSize) {
		try{
			struct request* incoming_request;
			incoming_request = (struct request*)myBuffer;
			request_t identifier = incoming_request->req_type;
			if (identifier == REQ_LOGIN && bytesRecvd==loginSize){//login
				struct request_login* incoming_login_request;
				incoming_login_request = (struct request_login*)myBuffer;
				std::string userName = std::string(incoming_login_request->req_username);
				std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv Request login " <<userName<<std::endl;
				bool addUser = true;
				if (currentUsers.size()>0) {
					for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
	     				if (remoteIPAddress.compare((*iter).myIPAddress) == 0&&remotePort==(*iter).myPort){
	     					sendError("*error , user is already logged in.",remoteIPAddress,remotePort);
	     					addUser = false;
	     				}
	     			}
	     		}
	     		if (addUser){
	     			userInfo newUser;
	     			newUser.myUserName = userName;
	     			newUser.myIPAddress = remoteIPAddress;
	     			newUser.myPort = remotePort;
	     			newUser.lastSeen = time (NULL);
	     			currentUsers.push_back(newUser);
	     		}			     	
			}
			else if (identifier == REQ_LOGOUT && bytesRecvd==logoutListKeepAliveSize){//logout
				if (currentUsers.size()>0) {
					int userSlot = findUserSlot(remoteIPAddress,remotePort);
					if (userSlot>=0){//user found in macro collection
		 				std::string userName = currentUsers[userSlot].myUserName;
						if ((remoteIPAddress.compare(currentUsers[userSlot].myIPAddress) == 0)&&remotePort==currentUsers[userSlot].myPort){
	  						std::cerr<<myIP<<":"<<myPort<<" "<< currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort << " recv Request Logout " <<userName<<std::endl;
	     					for (unsigned int y=0; y<currentUsers[userSlot].myChannels.size(); y++){
								server::leave(userName,currentUsers[userSlot].myChannels[y]);
							}
							//log out of server subscribed channels:
							for (unsigned int y=0; y<mySubscribedChannels.size(); y++){
								server::leave(currentUsers[userSlot].myUserName,mySubscribedChannels[y].myChannelName);
							}
							currentUsers.erase(currentUsers.begin()+userSlot);
	     					
	     				}
		     			
		     		}
	     		}
			}
			else if (identifier == REQ_JOIN && bytesRecvd == joinLeaveWhoSize){//join request
				struct request_join* incoming_join_request;
				incoming_join_request = (struct request_join*)myBuffer;
				std::string channelToJoin = std::string(incoming_join_request->req_channel);
				int userSlot = findUserSlot(remoteIPAddress,remotePort);
				if (userSlot>=0){//user found in macro collection
	 				
	 				std::string userName = currentUsers[userSlot].myUserName;
					std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv Request join " <<userName<<" "<<channelToJoin<<std::endl;
					bool channelFound=false;
					int position =-1;
					for (unsigned int x=0; x <mySubscribedChannels.size(); x++){
						if (channelToJoin.compare(mySubscribedChannels[x].myChannelName) == 0){
	     					channelFound=true;
	     					position = x;
	     					break;
	     				}
					}
	     			struct userInfo newUserInfo;
	 				newUserInfo.myUserName=userName;
					newUserInfo.myIPAddress=remoteIPAddress;
					newUserInfo.myPort=remotePort;
	     			if (!channelFound){//if channel not exist, add it
	     				struct channelInfo newChannel;
	     				newChannel.myChannelName = channelToJoin;
	     				newChannel.myUsers.push_back(newUserInfo);
	     				mySubscribedChannels.push_back(newChannel);
	     				//send s2s join to neighbors
	     				sendS2Sjoin(channelToJoin,myIP,myPort);
	    			}
	     			else{//channel was found
	     				bool userFound=false;//check if user exists in channel already
						for (unsigned int x=0; x <mySubscribedChannels[position].myUsers.size(); x++){
							if (userName.compare(mySubscribedChannels[position].myUsers[x].myUserName) == 0){//handled clientside in goodclient
		     					userFound=true;
		     					sendError("user already in channel",remoteIPAddress,remotePort);
		     					break;
							}
						}
						if (!userFound){
							mySubscribedChannels[position].myUsers.push_back(newUserInfo);
						}
	 				}
					currentUsers[userSlot].myActiveChannel = channelToJoin;
					currentUsers[userSlot].lastSeen = time (NULL);
	     		}
	     		else
	     			sendError("req join user not found",remoteIPAddress,remotePort);
			}
			else if (identifier == REQ_LEAVE && bytesRecvd == joinLeaveWhoSize){//leave request
				struct request_leave* incoming_leave_request;
				incoming_leave_request = (struct request_leave*)myBuffer;
				std::string channelToLeave = std::string(incoming_leave_request->req_channel);
				int userSlot = findUserSlot(remoteIPAddress,remotePort);
				if (userSlot>=0){//user found, erase from inside channel list
	 				std::string userName = currentUsers[userSlot].myUserName;
	 				std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv Request leave " <<userName<<" "<<channelToLeave<<std::endl;
					//remove from currentUsers/mychannels
					for (unsigned int x=0; x <currentUsers[userSlot].myChannels.size(); x++){
						if (channelToLeave.compare(currentUsers[userSlot].myChannels[x]) == 0){
	     					currentUsers[userSlot].myChannels.erase(currentUsers[userSlot].myChannels.begin() + x);
	     					break;
	     				}
	     			}
	     			//remove from mySubscribedchannels/myusers 
	     			int channelPosition = findChannelInfoPositionInVector(mySubscribedChannels,channelToLeave);
	     			if (channelPosition>-1){
	     				int userPosition = findUserInfoPositionInVector(mySubscribedChannels[channelPosition].myUsers,userName);
	     				if (userPosition>-1){
	     					mySubscribedChannels[channelPosition].myUsers.erase(mySubscribedChannels[channelPosition].myUsers.begin()+userPosition);
	     				}
	     				else{
	     					std::cerr << "can't find username to remove from channellist's users" <<std::endl;
	     					exit(EXIT_FAILURE);	
	     				}
	     			}
	     			else{
	     				std::cerr << "can't find channel in channellist to remove user from" <<std::endl;
	     				exit(EXIT_FAILURE);
	     			}
	     			leave(userName,channelToLeave);
					if (currentUsers[userSlot].myActiveChannel==channelToLeave)
						currentUsers[userSlot].myActiveChannel = "";
	 				currentUsers[userSlot].lastSeen = time (NULL);	
	 			}
	 			else
	     			sendError("user not found",remoteIPAddress,remotePort);
			}
			else if (identifier == REQ_SAY && bytesRecvd==sayRequestSize){//say request
				struct request_say* incoming_request_say;
				incoming_request_say = (struct request_say*)myBuffer;
				std::string channelToAnnounce = std::string(incoming_request_say->req_channel);
				std::string message = std::string(incoming_request_say->req_text);						
				int userSlot = findUserSlot(remoteIPAddress,remotePort);
	 			if (userSlot>=0){//user found
					std::cerr << myIP<<":"<<myPort<< " "<<currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort <<" recv Request say " <<currentUsers[userSlot].myUserName<<" "<<channelToAnnounce<<" "<<'\"'<< message<<'\"'<<std::endl;
					handleSay(remoteIPAddress,remotePort, channelToAnnounce, message,myBuffer);
				}
				else
		     		sendError("user not found",remoteIPAddress,remotePort);
							
			}
			else if (identifier == REQ_KEEP_ALIVE && bytesRecvd==logoutListKeepAliveSize){//keep alive
				int userSlot = findUserSlot(remoteIPAddress,remotePort);
	 			if (userSlot>=0){//user found
	 				std::cerr <<myIP<<":"<<myPort<<" "<< currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort <<" recv Request Keep Alive " <<currentUsers[userSlot].myUserName<<std::endl;
	 				currentUsers[userSlot].lastSeen = time (NULL);
	 			}
	 			else{
	 				sendError("*unknown user (please try reconnecting again in a few minutes.)",remoteIPAddress,remotePort);
	 			}
			}
			else if (identifier == REQ_LIST && bytesRecvd==logoutListKeepAliveSize){//list of channels
				int userSlot = findUserSlot(remoteIPAddress,remotePort);
				if (userSlot>=0){//user found
					if (neighborCount>0){
						//we will first set an internal record of the s2s_list_request
						struct listIDInfo newRecord;
						newRecord.origin = true;
						std::string tempString = generateID();
					    strcpy(newRecord.id,tempString.c_str());
					    newRecord.remoteAddress.sin_family = AF_INET;
						newRecord.remoteAddress.sin_port = remoteAddress.sin_port;
						newRecord.remoteAddress.sin_addr = remoteAddress.sin_addr;
						newRecord.remoteIPAddress = remoteIPAddress;
						newRecord.remotePort = remotePort;
						newRecord.timeStamp = time(NULL);
						newRecord.received =0;
						myRecentListRequests.push_back(newRecord);
						//broadcast
						sendS2Slist(myIP,myPort,newRecord.id);
						currentUsers[userSlot].lastSeen = time (NULL);
					}
					else{//no other servers, perform as in project 1
						int size = mySubscribedChannels.size();
						int reserveSize = sizeof(text_list)+sizeof(channel_info)*size;
						struct text_list* my_text_list = (text_list*)malloc(reserveSize);
						my_text_list->txt_type = TXT_LIST;
						my_text_list->txt_nchannels = mySubscribedChannels.size();
						for (unsigned int x=0; x<mySubscribedChannels.size(); x++){
							initBuffer((char*)my_text_list->txt_channels[x].ch_channel, CHANNEL_MAX);
							strcpy(my_text_list->txt_channels[x].ch_channel,mySubscribedChannels[x].myChannelName.c_str());
						}
						if (sendto(mySocket, my_text_list, reserveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
							perror("server sending channel list error");
							exit(EXIT_FAILURE);
						}
						free(my_text_list);
					}
	 			}
	 			else{
					sendError("user not found",remoteIPAddress,remotePort);
	 			}
			}
			else if (identifier == REQ_WHO && bytesRecvd==joinLeaveWhoSize){//list of people on certain channel
				int userSlot = findUserSlot(remoteIPAddress,remotePort);;
				if (userSlot>=0){
					struct request_who* incoming_request_who;
					incoming_request_who = (struct request_who*)myBuffer;
					std::string channelToQuery = std::string(incoming_request_who->req_channel);
					std::string userName = currentUsers[userSlot].myUserName;
		 			currentUsers[userSlot].lastSeen = time (NULL);
		 			//find given channel, check size
					int position =-1;
					for (unsigned int x=0; x<mySubscribedChannels.size(); x++){
						if (channelToQuery.compare(mySubscribedChannels[x].myChannelName)==0){
							position=x;
							break;
						}
					}
					if (position>=0){
						int size = mySubscribedChannels[position].myUsers.size();
						int reserveSize = sizeof(text_who)+sizeof(user_info)*size;
						struct text_who* my_text_who = (text_who*)malloc(reserveSize);
						my_text_who->txt_type = TXT_WHO;
						my_text_who->txt_nusernames = size;
						initBuffer(my_text_who->txt_channel, CHANNEL_MAX);
						strcpy(my_text_who->txt_channel,channelToQuery.c_str());
						for (unsigned int x=0; x<mySubscribedChannels[position].myUsers.size(); x++){
							initBuffer((char*)my_text_who->txt_users[x].us_username, USERNAME_MAX);
							strcpy(my_text_who->txt_users[x].us_username,mySubscribedChannels[position].myUsers[x].myUserName.c_str());
						}
						if (sendto(mySocket, my_text_who, reserveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
							perror("server sending who is error");
							exit(EXIT_FAILURE);
						}
						free(my_text_who);

					}
					else{
						sendError("*channel does not exist, join to create",remoteIPAddress,remotePort);
					}
				}
				else{
	 				sendError("*unknown user (please try again in a few minutes if reconnecting.)",remoteIPAddress,remotePort);
	 			}
			}
			else if (identifier == REQ_S2S_JOIN && bytesRecvd==s2sJoinLeaveSize){
				struct request_s2s_join* incoming_request_s2s_join;
				incoming_request_s2s_join = (struct request_s2s_join*)myBuffer;
				std::string channelToJoin = std::string(incoming_request_s2s_join->req_channel);
				std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv S2S Join " <<channelToJoin<<std::endl;					
				if (!amSubscribed(channelToJoin)){
					//then subscribe to channel if not:
					struct channelInfo newChannel;
	 				newChannel.myChannelName = channelToJoin;
	 				mySubscribedChannels.push_back(newChannel);
					//for each neighbor, send join request
					sendS2Sjoin(channelToJoin,remoteIPAddress,std::to_string(remotePort));
				}//else amSubscribed - do nothing
				else{
					//std::cerr <<" already subscribed, update server status and timestamp though
				}
				int serverPosition = findServerInfoPositionInVector(remoteIPAddress,remotePort);
				if (serverPosition>-1){
					int channelPosition = findStringPositionInVector(serverList[serverPosition].myChannels,channelToJoin);
					if (!(channelPosition>-1)){//don't double join
						serverList[serverPosition].myChannels.push_back(channelToJoin);//set calling neighbor to subscribed when recv join request
						serverList[serverPosition].myTimeStamps.push_back(time(NULL));
					}
					else
						serverList[serverPosition].myTimeStamps[channelPosition] = time(NULL);
				}	
			}
			else if (identifier == REQ_S2S_LEAVE && bytesRecvd==s2sJoinLeaveSize){
				struct request_s2s_leave* incoming_request_s2s_leave;
				incoming_request_s2s_leave = (struct request_s2s_leave*)myBuffer;
				std::string channel = std::string(incoming_request_s2s_leave->req_channel);
				std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv S2S Leave "<<channel<<std::endl;					
				int serverPosition = findServerInfoPositionInVector(remoteIPAddress,remotePort);
				if (serverPosition>-1){
					//leave channel for incoming server
					int channelPosition = findStringPositionInVector(serverList[serverPosition].myChannels,channel);
					if (channelPosition>-1){
						serverList[serverPosition].myChannels.erase(serverList[serverPosition].myChannels.begin()+channelPosition);
					}
				}
			}
			else if (identifier == REQ_S2S_SAY && bytesRecvd==sizeof(struct request_s2s_say)){
				struct request_s2s_say* incoming_request_s2s_say;
				incoming_request_s2s_say = (struct request_s2s_say*)myBuffer;
				std::string channel = std::string(incoming_request_s2s_say->req_channel);
				std::string userName = std::string(incoming_request_s2s_say->req_username);
				std::string message = std::string(incoming_request_s2s_say->req_text);
				char REQ_ID[8];
				strcpy(REQ_ID,incoming_request_s2s_say->req_ID);
				//determine if new or old id:
				int checkID = findID(std::string(REQ_ID));

				if (checkID>-1){
					//std::cerr<< "found duplicate id "<<REQ_ID<<std::endl;
					sendS2Sleave(channel,remoteIPAddress,std::to_string(remotePort),false);
				}
				else{
					struct requestIDInfo newID;
					initBuffer(newID.id,8);
					strcpy(newID.id,REQ_ID);
					newID.timeStamp = time(NULL);
					myRecentRequests.push_back(newID);
					std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv S2S Say " <<userName<<" "<<channel<<" "<<'\"'<<message<<'\"'<<std::endl;					
					int position = findChannelInfoPositionInVector(mySubscribedChannels,channel);
					bool noUsers = false;
					int  serverCount = 0;
					if (position!=-1){//we are subscribed to channel at [position]
						int numUsers = mySubscribedChannels[position].myUsers.size();
						if (numUsers >0){
							sendMessage(userName, channel, message);
						}
						else 
							noUsers=true;
						sendS2Ssay(userName,channel,message,remoteIPAddress,remotePort,myIP,myPortInt,true,myBuffer);	
						for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
							if (!(((*iter).myIPAddress==remoteIPAddress)&&(*iter).myPort==remotePort)){		//don't send right back to sender
								if (findStringPositionInVector((*iter).myChannels,channel)!=-1){//if server subscribed to channel
									serverCount++;
								}
							}
						}
						if (noUsers && serverCount==0){//then check for other servers in list subscribed to channel (besides sender)
							sendS2Sleave(channel,remoteIPAddress,std::to_string(remotePort),true);//if both then send s2s leave as response
						}
					}
					else{
						//std::cerr <<"..not subscribed to channel, ignoring."<<std::endl;
					}
				} 
				
			}
			else if (identifier == REQ_S2S_LIST){

				
				struct request_s2s_list* incoming_request_s2s_list;
				incoming_request_s2s_list = (struct request_s2s_list*)myBuffer;
				char REQ_ID[8];
				strcpy(REQ_ID,incoming_request_s2s_list->req_ID);
				unsigned int type = incoming_request_s2s_list->type;
				std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv S2S List "<<((type>0)?"Response":"Request")<<std::endl;					
				int channels = incoming_request_s2s_list->txt_nchannels;
				//determine if new or old id:
				int checkID = findlistID(std::string(REQ_ID));
				bool done=false;
				struct listIDInfo newRecord;
				bool found = checkID>-1;
				if (found){//found
					if (type==0){//request add list ID
						sendS2SlistSingle(remoteAddress,REQ_ID,2);
					}
					else if (type==2){//response returned empty meaning the servers channels were counted elsewhere, will increment received and check if done
						myRecentListRequests[checkID].received++;
					}
					else if (type==1){//returning data
						//get this data add to existing list ID data
						std::vector<std::string> newChannels;
						for (int x=0; x< channels; x++){
							std::string channel = incoming_request_s2s_list->txt_channels[x].ch_channel;
							int position = findStringPositionInVector(myRecentListRequests[checkID].channels,channel);
							if (position==-1){
								myRecentListRequests[checkID].channels.push_back(channel);
							}
						}
						myRecentListRequests[checkID].received++;
					}
					else{
						std::cerr << "unknown s2s list type error, type="<<type<<std::endl;
						exit(EXIT_FAILURE);
					}
					
					
					//check if done:
					if (neighborCount>0){//unnecessary on single servers
						unsigned int checkNeighbor = neighborCount-1;//since one neighbor was initial sender 
						bool origin = myRecentListRequests[checkID].origin;
						if (origin)
							checkNeighbor = neighborCount;//(except origin)
						if (myRecentListRequests[checkID].received >= checkNeighbor){
							done=true;
						}
						else{
							//std::cerr << "not done"<<std::endl;
						}
					}
					else{
						std::cerr << "neighborCount incorrect error"<<std::endl;//single server shouldn't have gotten a s2s list request
						exit(EXIT_FAILURE);
					}		
				}
				else{//not found
					if (type==0){
						struct listIDInfo newRecord;
						newRecord.origin = false;
						strcpy(newRecord.id,REQ_ID);
						newRecord.remoteAddress.sin_family = AF_INET;
						newRecord.remoteAddress.sin_port = remoteAddress.sin_port;
						newRecord.remoteAddress.sin_addr = remoteAddress.sin_addr;
						newRecord.remoteIPAddress = remoteIPAddress;
						newRecord.remotePort = remotePort;
						newRecord.timeStamp = time(NULL);
						newRecord.received = 0;
						myRecentListRequests.push_back(newRecord);
						if (newRecord.received >= neighborCount-1){
							done=true;
						}
						else{
							//std::cerr<<"rebroadcast"<<std::endl;
							sendS2Slist(myIP,myPort,REQ_ID);
						}
					}
					else{
						std::cerr << "type not 0, s2s list error"<<std::endl;
						exit(EXIT_FAILURE);
					}
				}
				if (done){ 
					if (!found){
							checkID = findlistID(std::string(REQ_ID));
					}
					bool origin = myRecentListRequests[checkID].origin;
					if (origin){//send txt list instead of s2s_req_list 
						addLocalChannels(checkID);
						int size = myRecentListRequests[checkID].channels.size();
						int reserveSize = sizeof(text_list)+sizeof(channel_info)*size;
						struct text_list* my_text_list = (text_list*)malloc(reserveSize);
						my_text_list->txt_type = TXT_LIST;
						my_text_list->txt_nchannels = size;
						for (unsigned int x=0; x<myRecentListRequests[checkID].channels.size(); x++){
							initBuffer((char*)my_text_list->txt_channels[x].ch_channel, CHANNEL_MAX);
							strcpy(my_text_list->txt_channels[x].ch_channel,myRecentListRequests[checkID].channels[x].c_str());
						}
						if (sendto(mySocket, (char*)my_text_list, reserveSize, 0, (struct sockaddr *)&myRecentListRequests[checkID].remoteAddress, sizeof(sockaddr_in))==-1){
							perror("server sending channel list error");
							exit(EXIT_FAILURE);
						}
						free(my_text_list);
					}
					else{	
						addLocalChannels(checkID);
						//and send back to initial sender
						unsigned int size = myRecentListRequests[checkID].channels.size();
						unsigned int reserveSize = sizeof(struct request_s2s_list)+sizeof(channel_info)*size;
								

						struct request_s2s_list* my_request_s2s_list= (request_s2s_list*)malloc(reserveSize);
						initBuffer(my_request_s2s_list->req_ID, ID_MAX);
						my_request_s2s_list->req_type = REQ_S2S_LIST;
						my_request_s2s_list->txt_nchannels = size ;
						my_request_s2s_list->type = 1;
						strcpy(my_request_s2s_list->req_ID,REQ_ID);
						for (unsigned int x=0; x<myRecentListRequests[checkID].channels.size(); x++){
							initBuffer((char*)my_request_s2s_list->txt_channels[x].ch_channel, CHANNEL_MAX);
							strcpy(my_request_s2s_list->txt_channels[x].ch_channel,myRecentListRequests[checkID].channels[x].c_str());
						}
						if (sendto(mySocket, (char*)my_request_s2s_list, reserveSize, 0, (struct sockaddr *)&myRecentListRequests[checkID].remoteAddress, sizeof(myRecentListRequests[checkID].remoteAddress))==-1){
							perror("server sending s2s list request to multiple servers");
							exit(EXIT_FAILURE);
						}
						free(my_request_s2s_list);

						}

				}
				else{
					//wait for another
				}

			

			}
		
		}
		catch (const std::exception& e) {
			std::cerr <<"exception caught reading packet, ignoring invalid request"<<std::endl;
		}
		
	}
}
void server::leave(std::string userName, std::string channelToLeave){
	int mastermySubscribedChannelsPosition =-1;
	int mySubscribedChannelsNamePosition =-1;
	for (unsigned int x=0; x <mySubscribedChannels.size(); x++){
		if (channelToLeave.compare(mySubscribedChannels[x].myChannelName) == 0){
				mastermySubscribedChannelsPosition=x;
				break;
		}
	}//this ensures the channel to leave is valid in our servers master list
	if (mastermySubscribedChannelsPosition>-1){
		for (unsigned int x=0; x <mySubscribedChannels[mastermySubscribedChannelsPosition].myUsers.size(); x++){
			if (userName.compare(mySubscribedChannels[mastermySubscribedChannelsPosition].myUsers[x].myUserName) == 0){
				mySubscribedChannelsNamePosition=x;
				break;
			}
		}
	}//this ensures the username is subscribed to the channel to leave
	if (mySubscribedChannelsNamePosition>-1){
		mySubscribedChannels[mastermySubscribedChannelsPosition].myUsers.erase(mySubscribedChannels[mastermySubscribedChannelsPosition].myUsers.begin()+mySubscribedChannelsNamePosition);
	}//this removes user from the channel and leaves remove channel if empty to s2sleave
}
//bool 
void server::checkPurge(time_t &purgeTime,time_t &keepAliveTime){
	time_t checkTime = time(NULL);	
	double seconds = difftime(checkTime,purgeTime);
	if (seconds>=serverTimeout){
		purge();
		purgeTime = time(NULL);	
		//return false;
	}
	seconds = difftime(checkTime,keepAliveTime);
	if (seconds>=keepAliveTimeout){
		renewJoins();
		keepAliveTime = time(NULL);
	}
	//return true;			
}
void server::serve(){
	neighborCount = serverList.size();
	setTimeout(mySocket,serverTimeout/25);
	time_t purgeTime = time (NULL);
	time_t keepAliveTimeout = time (NULL);
	//bool valgrind = true;
	while(true/*valgrind*/) {
		int bytesRecvd = 0;
		char myBuffer[BUFFERLENGTH];
		initBuffer(myBuffer,BUFFERLENGTH);
		bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
		if (bytesRecvd<=0){//recvfrom timeout : never going to happen unless all users stop pinging in
			//valgrind = 	
			checkPurge(purgeTime,keepAliveTimeout);
		}
		else if (bytesRecvd>0){
			//printf("received %d bytes\n", bytesRecvd);
			std::string remoteIPAddress=inet_ntoa(remoteAddress.sin_addr);
			short unsigned int remotePort =ntohs(remoteAddress.sin_port);
			//std::cerr << remoteIPAddress << ":" <<remotePort<<std::endl;
			handleRequest(myBuffer,bytesRecvd,remoteIPAddress,remotePort);//,inet_ntoa(remoteAddress.sin_addr),htons(remoteAddress.sin_port));	
			checkPurge(purgeTime,keepAliveTimeout);
		}		
	}
}
server::server(char* domain, char* port){
	neighborCount =0;
	myIP = std::string(domain);
	myPort = std::string(port);
	myPortInt = std::atoi(port);
	addressSize = sizeof(myAddress);
	bytesRecvd=0;
	bindSocket();
	randomPosition=0;
	seedRandom();	
}
void server::bindSocket(){
	mySocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (mySocket<0) {
		perror ("socket() failed");
		exit(EXIT_FAILURE);
	}
	myAddress.sin_family = AF_INET;
	struct hostent     *he;

	if ((he = gethostbyname(myIP.c_str())) == NULL) {
		puts("error resolving hostname..");
		exit(1);
	}
	memcpy(&myAddress.sin_addr, he->h_addr_list[0], he->h_length);
	myIP=inet_ntoa(myAddress.sin_addr);//replace localhost to 127.0.0.1 internally as in docs

	//myAddress.sin_addr.s_addr = inet_addr(myIP.c_str());//htonl(INADDR_ANY);//inet_addr("128.223.4.39");//
	myAddress.sin_port = htons(std::atoi(myPort.c_str()));	
	if (bind(mySocket, (struct sockaddr *)&myAddress, sizeof myAddress ) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}	
}
bool server::amSubscribed(std::string channel){
	int result = findChannelInfoPositionInVector(mySubscribedChannels,channel);
	if (result!=-1)
		return true;
	else
		return false;
}
void server::sendS2Sjoin(std::string channel,std::string senderIP, std::string senderPort){
	for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
		if (!(((*iter).myIPAddress==senderIP)&&(*iter).myPort==std::atoi(senderPort.c_str()))){
			int position = findStringPositionInVector((*iter).myChannels,channel);
			if (position==-1){
				(*iter).myChannels.push_back(channel);
				(*iter).myTimeStamps.push_back(time(NULL));
			}
			else
				(*iter).myTimeStamps[position]=time(NULL);	
			sendS2SjoinSingle((*iter).myIPAddress,(*iter).myPort,channel);
		}
	}
}
void server::sendS2SjoinSingle(std::string toIP, int toPort, std::string channel){
	struct request_s2s_join* my_request_s2s_join= new request_s2s_join;
	initBuffer(my_request_s2s_join->req_channel, CHANNEL_MAX);
	my_request_s2s_join->req_type = REQ_S2S_JOIN;
	strcpy(my_request_s2s_join->req_channel,channel.c_str());
	struct sockaddr_in remoteAddress;
	remoteAddress.sin_family = AF_INET;
	remoteAddress.sin_port = htons(toPort);
	//remoteAddress.sin_addr.s_addr = inet_addr(toIP.c_str());

	struct hostent     *he;


	if ((he = gethostbyname(toIP.c_str())) == NULL) {
		puts("error resolving hostname..");
		exit(1);
	}
	memcpy(&remoteAddress.sin_addr, he->h_addr_list[0], he->h_length);


	std::cerr << myIP <<":"<<myPort <<" "<<toIP<<":"<<toPort<< " send S2S Join " <<channel<<std::endl;					
	if (sendto(mySocket, my_request_s2s_join, s2sJoinLeaveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(sockaddr_in))==-1){
		perror("server sending s2s join request to multiple servers");
		exit(EXIT_FAILURE);
	}
	delete(my_request_s2s_join);
}
void server::sendS2SlistSingle(/*std::string toIP, std::string toPort,*/ struct sockaddr_in toAddress,char* id,int type){
	//std::cerr<<"sendS2SlistSingle called with id: "<<id<<std::endl;
	int reserveSize = sizeof(struct request_s2s_list);
		

	struct request_s2s_list* my_request_s2s_list= (request_s2s_list*)malloc(reserveSize);
	initBuffer(my_request_s2s_list->req_ID, ID_MAX);
	my_request_s2s_list->req_type = REQ_S2S_LIST;
	my_request_s2s_list->txt_nchannels = 0 ;
	my_request_s2s_list->type = type ;
	strcpy(my_request_s2s_list->req_ID,id);
	//std::cerr<<"my_request_s2s_list->req_ID id: "<<my_request_s2s_list->req_ID<<std::endl;
	
	if (sendto(mySocket, (char*)my_request_s2s_list, reserveSize, 0, (struct sockaddr *)&toAddress, sizeof(sockaddr_in))==-1){
		perror("server sending s2s list request to multiple servers");
		exit(EXIT_FAILURE);
	}
	free(my_request_s2s_list);
}
void server::sendS2Slist(std::string senderIP, std::string senderPort/*, struct sockaddr_in senderAddress, */,char* id){
	//std::cerr<<"sendS2Slist called with id: "<<id<<std::endl;
	//for each adj server, if not senderIP:port, send s2s list request
	int idSlot = findlistID(id);
	if (idSlot>-1){
		for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
			if ( !(((*iter).myIPAddress==senderIP)&&(*iter).myPort==atoi(senderPort.c_str()))  && !(((*iter).myIPAddress==myRecentListRequests[idSlot].remoteIPAddress)&&(*iter).myPort==myRecentListRequests[idSlot].remotePort)){
				//send single s2s list request
				sendS2SlistSingle((*iter).myAddress,id,0);
			}
			else{
				//std::cerr << "skipped sender........."<<std::endl;
			}
		}
	}
	else{
		std::cerr<<"cannot find id!"<<std::endl;
		exit(EXIT_FAILURE);
	}
}
void server::sendS2Sleave(std::string channel,std::string toIP, std::string toPort, bool close){//, char* buffer, bool useBuffer){
	struct request_s2s_leave* my_request_s2s_leave= new request_s2s_leave;
	initBuffer(my_request_s2s_leave->req_channel, CHANNEL_MAX);
	my_request_s2s_leave->req_type = REQ_S2S_LEAVE;
	strcpy(my_request_s2s_leave->req_channel,channel.c_str());
	struct sockaddr_in remoteAddress;
	remoteAddress.sin_family = AF_INET;
	remoteAddress.sin_port = htons(std::atoi(toPort.c_str()));
	struct hostent     *he;
	if ((he = gethostbyname(toIP.c_str())) == NULL) {
		puts("error resolving hostname..");
		exit(1);
	}
	memcpy(&remoteAddress.sin_addr, he->h_addr_list[0], he->h_length);
	std::cerr << myIP <<":"<<myPort <<" "<<toIP<<":"<<toPort<< " send S2S Leave " <<channel<<std::endl;					
	if (sendto(mySocket, (char*)my_request_s2s_leave, s2sJoinLeaveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
		perror("server sending s2s leave request to multiple servers");
		exit(EXIT_FAILURE);
	}
	if (close){
		int position = findChannelInfoPositionInVector(mySubscribedChannels,channel);
		if (position!=-1){
			mySubscribedChannels.erase(mySubscribedChannels.begin()+position);
			
		}
	}
	delete(my_request_s2s_leave);
}
void server::sendS2Ssay(std::string fromUser, std::string toChannel,std::string message,std::string senderIP, int senderPort,std::string fromIP, int fromPort, bool resend, char* buf){
	struct request_s2s_say* my_request_s2s_say= new request_s2s_say;
	initBuffer(my_request_s2s_say->req_username, USERNAME_MAX);
	initBuffer(my_request_s2s_say->req_channel, CHANNEL_MAX);
	initBuffer(my_request_s2s_say->req_text, SAY_MAX);
	initBuffer(my_request_s2s_say->req_ID, ID_MAX);
	my_request_s2s_say->req_type = REQ_S2S_SAY;
	strcpy(my_request_s2s_say->req_channel,toChannel.c_str());
	strcpy(my_request_s2s_say->req_username,fromUser.c_str());
	strcpy(my_request_s2s_say->req_text,message.c_str());
    std::string tempString=generateID();
    //add to local records 
    struct requestIDInfo newID;
	strcpy(newID.id,tempString.c_str());
	newID.timeStamp = time(NULL);
	myRecentRequests.push_back(newID);
    strcpy(my_request_s2s_say->req_ID,tempString.c_str());
	for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
		if ( !(((*iter).myIPAddress==senderIP)&&(*iter).myPort==senderPort) && !(((*iter).myIPAddress==fromIP)&&(*iter).myPort==fromPort)) {		//don't send right back to sender
			if (findStringPositionInVector((*iter).myChannels,toChannel)!=-1){//if server subscribed to channel
				struct sockaddr_in remoteAddress; //should save these in serverInfo as per sample code
				remoteAddress.sin_family = AF_INET;
				remoteAddress.sin_port = htons((*iter).myPort);
				//remoteAddress.sin_addr.s_addr = inet_addr((*iter).myIPAddress.c_str());
				
				
				//wrap me in function************
				struct hostent     *he;
				if ((he = gethostbyname((*iter).myIPAddress.c_str())) == NULL) {
					puts("error resolving hostname..");
					exit(1);
				}
				memcpy(&remoteAddress.sin_addr, he->h_addr_list[0], he->h_length);


				std::cerr << myIP <<":"<<myPort <<" "<<(*iter).myIPAddress<<":"<<(*iter).myPort<< " send S2S Say " <<fromUser<<" "<<toChannel<<" "<<"\""<<message<<"\""<<std::endl;					
				if (!resend){
					if (sendto(mySocket, (char*)my_request_s2s_say, sizeof(struct request_s2s_say), 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
						perror("server sending s2s say request to multiple servers");
						exit(EXIT_FAILURE);
					}
				}
				else{
					if (sendto(mySocket, buf , sizeof(struct request_s2s_say), 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
						perror("server sending s2s say request to multiple servers");
						exit(EXIT_FAILURE);
					}	
				}
			}//else skip server
			else{
				//std::cerr << "server not subscribed to "<< toChannel<<std::endl;
			}
		}
		else{
			//std::cerr << "server skipped.. "<<std::endl;
		}
	}
	delete(my_request_s2s_say);
}
void server::renewJoins(){//runs once per minute
	for (unsigned int x=0; x<mySubscribedChannels.size(); x++){//for each channel x in my subscribed channels
		std::string channel = mySubscribedChannels[x].myChannelName;
		for (unsigned int y=0; y<serverList.size(); y++){//for each adjacent server
			//if server is subscribed to channel x, renew join
			int position =findStringPositionInVector(serverList[y].myChannels,channel);
			if (position>-1){
				serverList[y].myTimeStamps[position] = time(NULL);
				sendS2SjoinSingle(serverList[y].myIPAddress,serverList[y].myPort,channel);
			}
		}
	}
}
void server::handleSay(std::string fromIP, int fromPort, std::string channel, std::string message,char* buf){
	int userSlot = findUserSlot(fromIP,fromPort);
	if (userSlot>=0){//sending user found, proceed.
		std::string userName=currentUsers[userSlot].myUserName;
		sendMessage(userName, channel, message);
		currentUsers[userSlot].lastSeen = time (NULL);
		//s2s say
		if (!isFromServer(fromIP,fromPort))//if first issued from a user, make a new id for s2s say else resend
			sendS2Ssay(currentUsers[userSlot].myUserName,channel,message,myIP,myPortInt,fromIP,fromPort,false,buf);	
		else
			sendS2Ssay(currentUsers[userSlot].myUserName,channel,message,myIP,myPortInt,fromIP,fromPort,true,buf);	
	}
	else{
		std::cerr<<"found no user at ip:port "<< fromIP<<":"<<fromPort<< " to send from "<<std::endl;
		exit(EXIT_FAILURE);
	}
}
int server::findServerInfoPositionInVector(std::string ip, int port){
	int found = -1;
	for (unsigned int x=0; x<serverList.size(); x++) {
		if (serverList[x].myIPAddress==ip && serverList[x].myPort==port){
			found=x;
			break;
		}
	}
	return found;
}
int server::findID(std::string input){
	int found = -1;
	for (unsigned int x=0; x<myRecentRequests.size(); x++) {
		time_t checkTime = time(NULL);	
		double seconds = difftime(checkTime,myRecentRequests[x].timeStamp);
		if (seconds>serverTimeout){
			myRecentRequests.erase(myRecentRequests.begin()+x);
		}
		else if (strcmp(input.c_str(),myRecentRequests[x].id)==0){
			found=x;
			break;
		}
	}
	return found;
}
int server::findlistID(std::string input){
	int found = -1;
	for (unsigned int x=0; x<myRecentListRequests.size(); x++) {
		time_t checkTime = time(NULL);	
		double seconds = difftime(checkTime,myRecentListRequests[x].timeStamp);
		if (seconds>serverTimeout){
			myRecentListRequests.erase(myRecentListRequests.begin()+x);
		}
		else if (strcmp(input.c_str(),myRecentListRequests[x].id)==0){
			found=x;
			break;
		}
	}
	return found;
}
bool server::isFromServer(std::string ip, int port){
	if (findServerInfoPositionInVector(ip,port)>-1)
		return true;
	else
		return false;
}		
void server::seedRandom(){
	char myRandomData[8];
	ssize_t result;
	int randomData = open("/dev/random", O_RDONLY);
	if (randomData < 0)
	{
	    std::cerr << "error reading random data "<< std::endl;
	    exit(EXIT_FAILURE);
	}
	else
	{
        result = read(randomData, myRandomData, sizeof myRandomData);
        if (result < 0)
        {
            std::cerr << "error reading random data "<< std::endl;
    		exit(EXIT_FAILURE);
        }
    	srand((unsigned long long)myRandomData);
		close(randomData);
	}
}
std::string server::generateID(){
	std::string tempString;
    for (int v=0; v<8; v++){
    	unsigned int temp =   rand()%9999999;
    	tempString = std::to_string(temp);
    }
    while (tempString.length()<7){
    	tempString = "0"+tempString;
    }
    return tempString;
}
void server::addLocalChannels(int id){
	//mix in my channels to list channels 
	for (unsigned int x=0; x< mySubscribedChannels.size(); x++){
		std::string channel = mySubscribedChannels[x].myChannelName;
		int position=-1;
		position = findStringPositionInVector(myRecentListRequests[id].channels,channel);
		if (position==-1){
			myRecentListRequests[id].channels.push_back(channel);
		}
	}					
}