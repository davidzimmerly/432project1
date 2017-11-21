#include "server.h"
//cis432 fall 2017 David Zimmerly
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
void server::purgeUsers(){
	//std::cerr << "initiating purgeUsers"<< std::endl;
	time_t timeNow = time(NULL);
	for (unsigned int x=0; x<currentUsers.size(); x++){
		//std::cerr << "checking user "<<x<< std::endl;
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
			//std::cerr << "server logs " <<currentUsers[x].myUserName <<" out during purge." << std::endl;
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
}
void server::sendMessage(std::string fromUser/*, int userPosition*/, std::string toChannel, std::string message){
	std::cerr<<"sendMessage"<<std::endl;
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
		for(unsigned int x =0 ; x < listOfUsers.size(); x++){
			struct sockaddr_in remoteAddress;
			remoteAddress.sin_family = AF_INET;
			remoteAddress.sin_port = htons(listOfUsers[x].myPort);
			remoteAddress.sin_addr.s_addr = inet_addr(listOfUsers[x].myIPAddress.c_str());\
			std::cerr<< " sending mail to "<<listOfUsers[x].myUserName<<" at ip "<<listOfUsers[x].myIPAddress<< " on port "<< listOfUsers[x].myPort<<std::endl;
			if (sendto(mySocket, (char*)my_text_say, saySize, 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
				perror("server sending mail to multiple users");
				exit(EXIT_FAILURE);
			}
		}
		//send s2s say
		//only if server in channel			
		
		delete(my_text_say);
	}
}
void server::handleRequest(char* myBuffer,int bytesRecvd,std::string remoteIPAddress, int remotePort){
	if (bytesRecvd>=BUFFERLENGTH){
		std::cerr << "*buffer overflow, ignoring request" << std::endl;
		sendError("*buffer overflow error",remoteIPAddress,remotePort);
	}
	else if (bytesRecvd >= logoutListKeepAliveSize) {
		struct request* incoming_request;
		incoming_request = (struct request*)myBuffer;
		request_t identifier = incoming_request->req_type;
		if (identifier == REQ_LOGIN && bytesRecvd==loginSize){//login
			struct request_login* incoming_login_request;
			incoming_login_request = (struct request_login*)myBuffer;
			std::string userName = std::string(incoming_login_request->req_username);
			std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv Request login " <<userName<<std::endl;
			//std::cerr << "server: " << userName <<" logs in" << std::endl;
			bool addUser = true;
			if (currentUsers.size()>0) {
				for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
     				if (/*userName.compare((*iter).myUserName) == 0&&*/remoteIPAddress.compare((*iter).myIPAddress) == 0&&remotePort==(*iter).myPort){
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
  						std::cerr << currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort <<" "<<myIP<<":"<<myPort<< " recv Request Logout " <<userName<<std::endl;
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
 				for (unsigned int x=0; x <currentUsers[userSlot].myChannels.size(); x++){
					if (channelToLeave.compare(currentUsers[userSlot].myChannels[x]) == 0){
     					currentUsers[userSlot].myChannels.erase(currentUsers[userSlot].myChannels.begin() + x);
     					break;
     				}
     			}
     			for (unsigned int y=0; y<currentUsers[userSlot].myChannels.size(); y++){
					server::leave(userName,currentUsers[userSlot].myChannels[y]);
				}
				//log out of server subscribed channels:
				for (unsigned int y=0; y<mySubscribedChannels.size(); y++){
					server::leave(currentUsers[userSlot].myUserName,mySubscribedChannels[y].myChannelName);
				}
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
				std::cerr << currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort <<" "<<myIP<<":"<<myPort<< " recv Request say " <<currentUsers[userSlot].myUserName<<" "<<channelToAnnounce<<" "<<'\"'<< message<<'\"'<<std::endl;
				handleSay(remoteIPAddress,remotePort, channelToAnnounce, message);
			}
			else
	     		sendError("user not found",remoteIPAddress,remotePort);
						
		}
		else if (identifier == REQ_KEEP_ALIVE && bytesRecvd==logoutListKeepAliveSize){//keep alive
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
 			if (userSlot>=0){//user found
 				std::cerr << currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort <<" "<<myIP<<":"<<myPort<< " recv Request Keep Alive " <<currentUsers[userSlot].myUserName<<std::endl;
 				currentUsers[userSlot].lastSeen = time (NULL);
 			}
 			else{
 				sendError("*unknown user (please try reconnecting again in a few minutes.)",remoteIPAddress,remotePort);
 			}
		}
		else if (identifier == REQ_LIST && bytesRecvd==logoutListKeepAliveSize){//list of channels
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
			if (userSlot>=0){//user found
	 			
				int size = mySubscribedChannels.size();
				int reserveSize = sizeof(text_list)+sizeof(channel_info)*size-1;
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
				currentUsers[userSlot].lastSeen = time (NULL);
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
					int reserveSize = sizeof(text_who)+sizeof(user_info)*size-1;
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
				else
					sendError("*channel does not exist, join to create",remoteIPAddress,remotePort);
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

				//std::cerr << myIP <<":"<<myPort<< " s2s adding channel " << channelToJoin <<std::endl;
 				struct channelInfo newChannel;
 				newChannel.myChannelName = channelToJoin;
 				mySubscribedChannels.push_back(newChannel);
				int serverPosition = findServerInfoPositionInVector(remoteIPAddress,remotePort);
				if (serverPosition>-1){
					serverList[serverPosition].myChannels.push_back(channelToJoin);//set calling neighbor to subscribed when recv join request


				}	
				//for each neighbor, send join request
			
				sendS2Sjoin(channelToJoin,remoteIPAddress,std::to_string(remotePort));
			}//else amSubscribed - do nothing

		}
		else if (identifier == REQ_S2S_LEAVE && bytesRecvd==s2sJoinLeaveSize){

			//std::cerr << "received s2s leave request" << std::endl;
			struct request_s2s_leave* incoming_request_s2s_leave;
			incoming_request_s2s_leave = (struct request_s2s_leave*)myBuffer;
			std::string channel = std::string(incoming_request_s2s_leave->req_channel);

			int serverPosition = findServerInfoPositionInVector(remoteIPAddress,remotePort);
			if (serverPosition>-1){
				//leave channel for incoming server
				int channelPosition = findStringPositionInVector(serverList[serverPosition].myChannels,channel);
				if (channelPosition>-1){
					serverList[serverPosition].myChannels.erase(serverList[serverPosition].myChannels.begin()+channelPosition);
				}

			}
			
			


		}
		else if (identifier == REQ_S2S_SAY/* && bytesRecvd==s2sSaySize*/){
			std::cerr << "received s2s say request" << std::endl;
			

			struct request_s2s_say* incoming_request_s2s_say;
			incoming_request_s2s_say = (struct request_s2s_say*)myBuffer;
			std::string channel = std::string(incoming_request_s2s_say->req_channel);
			std::string userName = std::string(incoming_request_s2s_say->req_username);
			std::string message = std::string(incoming_request_s2s_say->req_text);
			char REQ_ID[8];
			strcpy(REQ_ID,incoming_request_s2s_say->req_ID);
			//verify new id:
			int checkID = findID(REQ_ID);

			if (checkID>-1){
				std::cerr<< "found duplicate id"<<std::endl;
				time_t checkTime = time(NULL);	
				double seconds = difftime(checkTime,myRecentRequests[checkID].timeStamp);
				if (seconds>serverTimeout){
					myRecentRequests.erase(myRecentRequests.begin()+checkID);
				}
			}
			else{
				//new id:
				/*strcpy(incoming_request_s2s_say->req_ID, REQ_ID);*/
				struct requestIDInfo newID;
				initBuffer(newID.id,8);
				strcpy(newID.id,REQ_ID);
				newID.timeStamp = time(NULL);
				myRecentRequests.push_back(newID);
				std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv S2S Say " <<userName<<" "<<channel<<" "<<'\"'<<message<<'\"'<<std::endl;					
				//check for users in channel*******
				int position = findChannelInfoPositionInVector(mySubscribedChannels,channel);
				bool noUsers = false;
				int  serverCount = 0;
				if (position!=-1){//we are subscribed to channel at [position]

					int numUsers = mySubscribedChannels[position].myUsers.size();
					if (numUsers >0){
						//make say packet and send to each subscribed user like in handle req_say

						for (int x=0; x<numUsers ; x++){
							//for each user in mySubscribedChannels[position].myUsers, send the packet
							int userSlot = findUserInfoPositionInVector(currentUsers,mySubscribedChannels[position].myUsers[x].myUserName);		
							if (userSlot>=0){//user found
								std::cerr << currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort <<" "<<myIP<<":"<<myPort<< " recv Request say " <<currentUsers[userSlot].myUserName<<" "<<channel<<" "<<'\"'<< message<<'\"'<<std::endl;
				 						

								handleS2SSay(currentUsers[userSlot].myUserName, channel, message, userName);
							
							}
						}
					}
					else 
						noUsers=true;

					for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
						if (!(((*iter).myIPAddress==remoteIPAddress)&&(*iter).myPort==remotePort)){		//don't send right back to sender
							if (findStringPositionInVector((*iter).myChannels,channel)!=-1){//if server subscribed to channel
								serverCount++;
							}
						}
					}

					if (noUsers && serverCount==0){//then check for other servers in list subscribed to channel (besides sender)
						sendS2Sleave(channel,remoteIPAddress,std::to_string(remotePort));//if both then send s2s leave as response
					}
							

				
				}
			
			
			} 
			
		}
	}
}
void server::leave(std::string userName, std::string channelToLeave){
	std::cerr << "server: " << userName <<" leaves channel " << channelToLeave <<std::endl;
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
		if (mySubscribedChannels[mastermySubscribedChannelsPosition].myUsers.size()==0 /*&& channelToLeave!="Common"*/){
			mySubscribedChannels.erase(mySubscribedChannels.begin()+mastermySubscribedChannelsPosition);
			std::cerr << "server: removing empty channel "<<channelToLeave<<std::endl;
			//send s2s leave
		}
	}//this removes user from the channel and then removes channel if empty
}
//bool 
void server::checkPurge(time_t &purgeTime){
	time_t checkTime = time(NULL);	
	double seconds = difftime(checkTime,purgeTime);
	if (seconds>=serverTimeout){
		purgeUsers();
		purgeTime = time(NULL);	
		//return false;
	}
	//return true;			
}
void server::serve(){
	setTimeout(mySocket,serverTimeout/25);
	time_t purgeTime = time (NULL);
	//bool valgrind = true;
	while(true/*valgrind*/) {
		int bytesRecvd = 0;
		char myBuffer[BUFFERLENGTH];
		initBuffer(myBuffer,BUFFERLENGTH);
		//std::cerr <<"bound IP address:" << myIP << " waiting on Port: "<< myPort <<std::endl;
		//printf("waiting on port %d\n", port.c_str);
		bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
		if (bytesRecvd<=0){//recvfrom timeout : never going to happen unless all users stop pinging in
			/*valgrind = */
			checkPurge(purgeTime);
		}
		else if (bytesRecvd>0){
			//printf("received %d bytes\n", bytesRecvd);
			
			std::string remoteIPAddress=inet_ntoa(remoteAddress.sin_addr);
			short unsigned int remotePort =ntohs(remoteAddress.sin_port);
			//std::cerr << remoteIPAddress << ":" <<remotePort<<std::endl;
			handleRequest(myBuffer,bytesRecvd,remoteIPAddress,remotePort);//,inet_ntoa(remoteAddress.sin_addr),htons(remoteAddress.sin_port));	
			checkPurge(purgeTime);
		}		
	}
}
server::server(char* domain, char* port){
	myIP = std::string(domain);
	myPort = std::string(port);
	myPortInt = std::atoi(port);
	addressSize = sizeof(myAddress);
	bytesRecvd=0;
	bindSocket();
	char myRandomData[8];
    size_t randomDataLen = 0;
    int randomData = open("/dev/random", O_RDONLY);
	if (randomData < 0)
	{
	    std::cerr << "error reading random data "<< std::endl;
	    exit(EXIT_FAILURE);
	}
	else
	{
	
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
    //std::cerr << "random data (int):"<<std::endl;
    
    //	std::cerr<<(unsigned long long)myRandomData<<" ";
    	srand((unsigned long long)myRandomData);
    
    //std::cerr<<std::endl;
    //strcpy(my_request_s2s_say->req_ID,myRandomData);
    	close(randomData);
    }
	//serve();
}

void server::bindSocket(){
	mySocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (mySocket<0) {
		perror ("socket() failed");
		exit(EXIT_FAILURE);
	}
	//struct channelInfo newChannel;
	//newChannel.myChannelName = "Common";
	//mySubscribedChannels.push_back(newChannel);
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = inet_addr(myIP.c_str());//htonl(INADDR_ANY);//inet_addr("128.223.4.39");//
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
void server::sendS2Sjoin(std::string channel,std::string senderIP, std::string senderPort){//, char* buffer, bool useBuffer){
	//std::cerr << myIP <<":"<<myPort<< " send sendS2Sjoin "<< "sender:"<<senderIP<<"senderport:"<<senderPort<<std::endl;
	struct request_s2s_join* my_request_s2s_join= new request_s2s_join;
	initBuffer(my_request_s2s_join->req_channel, CHANNEL_MAX);
	my_request_s2s_join->req_type = REQ_S2S_JOIN;
	strcpy(my_request_s2s_join->req_channel,channel.c_str());



	for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
		if (!(((*iter).myIPAddress==senderIP)&&(*iter).myPort==std::atoi(senderPort.c_str()))){
			//std::cerr<< (*iter).myIPAddress <<":"<<(*iter).myPort<<" not a match to sender"<<std::endl;
			struct sockaddr_in remoteAddress;
			remoteAddress.sin_family = AF_INET;
			remoteAddress.sin_port = htons((*iter).myPort);
			remoteAddress.sin_addr.s_addr = inet_addr((*iter).myIPAddress.c_str());
			
			std::cerr << myIP <<":"<<myPort <<" "<<(*iter).myIPAddress<<":"<<(*iter).myPort<< " send S2S Join " <<channel<<std::endl;					

			(*iter).myChannels.push_back(channel);
			if (sendto(mySocket, (char*)my_request_s2s_join, s2sJoinLeaveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
				perror("server sending s2s join request to multiple servers");
				exit(EXIT_FAILURE);
			}
		}
	}
	delete(my_request_s2s_join);

}
//almost same as above function, probably could combine
void server::sendS2Sleave(std::string channel,std::string toIP, std::string toPort){//, char* buffer, bool useBuffer){
	//std::cerr << myIP <<":"<<myPort<< " send sendS2Sleave"<<std::endl;
	struct request_s2s_leave* my_request_s2s_leave= new request_s2s_leave;
	initBuffer(my_request_s2s_leave->req_channel, CHANNEL_MAX);
	my_request_s2s_leave->req_type = REQ_S2S_LEAVE;
	strcpy(my_request_s2s_leave->req_channel,channel.c_str());
	


	struct sockaddr_in remoteAddress;
	remoteAddress.sin_family = AF_INET;
	remoteAddress.sin_port = htons(std::atoi(toPort.c_str()));
	remoteAddress.sin_addr.s_addr = inet_addr(toIP.c_str());
	
	std::cerr << myIP <<":"<<myPort <<" "<<toIP<<":"<<toPort<< " send S2S Leave " <<channel<<std::endl;					

	if (sendto(mySocket, (char*)my_request_s2s_leave, s2sJoinLeaveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
		perror("server sending s2s leave request to multiple servers");
		exit(EXIT_FAILURE);
	}
	delete(my_request_s2s_leave);

}
void server::sendS2Ssay(std::string fromUser, std::string toChannel,std::string message,std::string senderIP, int senderPort){
	std::cerr<<"sends2ssay"<<std::endl;
	struct request_s2s_say* my_request_s2s_say= new request_s2s_say;
	initBuffer(my_request_s2s_say->req_username, USERNAME_MAX);
	initBuffer(my_request_s2s_say->req_channel, CHANNEL_MAX);
	initBuffer(my_request_s2s_say->req_text, SAY_MAX);
	initBuffer(my_request_s2s_say->req_ID, ID_MAX);
	my_request_s2s_say->req_type = REQ_S2S_SAY;
	strcpy(my_request_s2s_say->req_channel,toChannel.c_str());
	strcpy(my_request_s2s_say->req_username,fromUser.c_str());
	strcpy(my_request_s2s_say->req_text,message.c_str());
	
    char myRandomData[8];
    for (int v=0; v<8; v++){
    	myRandomData[v] = rand()%255;
    }

    
    std::cerr<<(unsigned long long)myRandomData<<std::endl;
    strcpy(my_request_s2s_say->req_ID,myRandomData);
	    
	




	for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
		if (!(((*iter).myIPAddress==senderIP)&&(*iter).myPort==senderPort)){		//don't send right back to sender
			std::cerr << "s2s say iterating.. found another server PORT: "<<(*iter).myPort<<std::endl; 
			if (findStringPositionInVector((*iter).myChannels,toChannel)!=-1){//if server subscribed to channel
				struct sockaddr_in remoteAddress; //should save these in serverInfo as per sample code
				remoteAddress.sin_family = AF_INET;
				remoteAddress.sin_port = htons((*iter).myPort);
				remoteAddress.sin_addr.s_addr = inet_addr((*iter).myIPAddress.c_str());
				std::cerr << myIP <<":"<<myPort <<" "<<(*iter).myIPAddress<<":"<<(*iter).myPort<< " send S2S Say " <<my_request_s2s_say->req_username<<" "<<my_request_s2s_say->req_channel<<" "<<"\""<<my_request_s2s_say->req_text<<"\""<<std::endl;					

					if (sendto(mySocket, (char*)my_request_s2s_say, s2sSaySize, 0, (struct sockaddr *)&remoteAddress, sizeof(struct sockaddr_in))==-1){
					perror("server sending s2s say request to multiple servers");
					exit(EXIT_FAILURE);
				}
			}//else skip server
			else{
				std::cerr << "server not subscribed to "<< toChannel<<std::endl;
			}
		}
	}
	delete(my_request_s2s_say);

}
void server::handleSay(std::string fromIP, int fromPort, std::string channel, std::string message){
	std::cerr<<"starting handleSay"<<std::endl;
	int userSlot = findUserSlot(fromIP,fromPort);
	if (userSlot>=0){//sending user found, proceed.

			std::string userName=currentUsers[userSlot].myUserName;
			std::cerr<<"sending mail to "<<userName<<std::endl;
			//std::cerr << fromIP <<":"<< fromPort <<" "<<myIP<<":"<<myPort<< " recv Request Say " <<userName<<" "<< std::endl;
			sendMessage(userName/*, userSlot*/, channel, message);
		
		currentUsers[userSlot].lastSeen = time (NULL);
		//s2s say
		sendS2Ssay(currentUsers[userSlot].myUserName,channel,message,myIP,myPortInt);
	}
	else{
		std::cerr<<"found no user at ip/port "<< fromIP<<"/"<<fromPort<< " to send to "<<std::endl;
	}

}
void server::handleS2SSay(std::string user, std::string channel, std::string message, std::string fromUser){
	int userSlot = findUserInfoPositionInVector(currentUsers,user);
	if (userSlot>=0){//sending user found, proceed.
			std::string userName=currentUsers[userSlot].myUserName;
//			std::cerr << fromIP <<":"<< fromPort <<" "<<myIP<<":"<<myPort<< " recv Request Say " <<userName<<" "<< std::endl;
			sendMessage(fromUser/*, userSlot*/, channel, message);
		
		currentUsers[userSlot].lastSeen = time (NULL);
		//s2s say
	}
	else{
		std::cerr<<"found no user "<< user << " to send to "<<std::endl;
	}
}



int main (int argc, char *argv[]){
	if (argc<3 || argc%2==0){
		std::cerr/*argv[1]*/<<"Usage: ./server domain_name port_num neighbor_server_1_IP neighbor_server_1_Port... neighbor_serverb_X_IP neighbor_serverb_X_Port"<<std::endl;
		exit(EXIT_FAILURE);
	}
	int count =5;//if >=5 parameters, start adding adjacent servers
	server* myServer = new server(argv[1],argv[2]);
	
	while (argc>=count){
		
		struct serverInfo newNeighbor;
		newNeighbor.myIPAddress = argv[count-2];
		newNeighbor.myPort =  std::atoi(argv[count-1]);//lets do some input validation at some point
		//std::cerr << "setting new neighbor ip "<< argv[count-2] << std::endl;
		//std::cerr << "setting new neighbor port "<< std::atoi(argv[count-1]) << std::endl;
		myServer->serverList.push_back(newNeighbor);
		count +=2; 
	


	}
	myServer->serve();
	delete(myServer);
	return 0;
}