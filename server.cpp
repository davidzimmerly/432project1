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
	//std::cerr<< "received message from user "<< fromUser<< " to channel "<< toChannel <<" msg: "<<message<<std::endl;
	

	//std::cerr << myIP <<":"<<myPort <<" "<<currentUsers[userPosition].myIPAddress<<":"<<currentUsers[userPosition].myPort<< " send  " <<channelToJoin<<std::endl;					
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
			//std::cerr<< " sending mail to "<<listOfUsers[x].myUserName<<" at ip "<<listOfUsers[x].myIPAddress<< " on port "<< listOfUsers[x].myPort<<std::endl;
			if (sendto(mySocket, (char*)my_text_say, saySize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
				perror("server sending mail to multiple users");
				exit(EXIT_FAILURE);
			}
		}
		//send s2s say			
		sendSRSsay(fromUser,toChannel,message,"",0);//we won't check this ip/port since should be a user not server
				
		//std::cerr << myIP <<":"<<myPort <<" "<<currentUsers[userPosition].myIPAddress<<":"<<currentUsers[userPosition].myPort<< " send  " <<channelToJoin<<std::endl;					
		delete(my_text_say);
	}
}
void server::handleRequest(char* myBuffer,int bytesRecvd,std::string remoteIPAddress, int remotePort){
	//std::string remoteIPAddress=inet_ntoa(remoteAddress.sin_addr);
	//int remotePort =htons(remoteAddress.sin_port);
	std::cerr<< "remotepo0rt:"<< remotePort<<std::endl;
	if (bytesRecvd>=BUFFERLENGTH){
		std::cerr << "*buffer overflow, ignoring request" << std::endl;
		sendError("*buffer overflow error",remoteIPAddress,remotePort);
	}
	else if (bytesRecvd >= logoutListKeepAliveSize) {
		struct request* incoming_request;
		incoming_request = (struct request*)myBuffer;
		request_t identifier = incoming_request->req_type;
		socklen_t remoteIPAddressSize = sizeof(remoteAddress);
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
     			//newUser.myChannels.push_back("Common");
     			//newUser.myActiveChannel = "Common";
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
     				//std::cerr << "adding channel " << channelToJoin <<std::endl;
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
//						std::cerr << "server: " << userName <<" joins channel " << channelToJoin <<std::endl;
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
				

     			//server::leave(userName,channelToLeave);
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
			std::string textField = std::string(incoming_request_say->req_text);						
			say(remoteIPAddress,remotePort, channelToAnnounce, textField);
			/*int userSlot = findUserSlot(remoteIPAddress,remotePort);
			if (userSlot>=0){//user found
 				std::string userName=currentUsers[userSlot].myUserName;
// 				std::cerr << "say request received from "<< "userName: " <<userName << " for channel " << channelToAnnounce << std::endl;
     			std::cerr << currentUsers[userSlot].myIPAddress <<":"<< currentUsers[userSlot].myPort <<" "<<myIP<<":"<<myPort<< " recv Request Say " <<userName<<" "<< std::endl;
     			sendMessage(userName, channelToAnnounce, textField);
				
				currentUsers[userSlot].lastSeen = time (NULL);
				//s2s say
			}
			else
     			sendError("user not found",remoteIPAddress,remotePort);*/
			
		}
		else if (identifier == REQ_KEEP_ALIVE && bytesRecvd==logoutListKeepAliveSize){//keep alive
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
 			if (userSlot>=0){//user found
 				//std::cerr <<"keep alive from "<< currentUsers[userSlot].myUserName << std::endl;
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
				if (sendto(mySocket, my_text_list, reserveSize, 0, (struct sockaddr *)&remoteAddress, remoteIPAddressSize)==-1){
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
			//int result = findChannelInfoPositionInVector(mySubscribedChannels,channelToJoin);
			//std::cerr << "received s2s join request for channel " << channelToJoin << std::endl;
			std::cerr << myIP <<":"<<myPort <<" "<<remoteIPAddress<<":"<<remotePort<< " recv S2S Join " <<channelToJoin<<std::endl;					
			
			//recv join request
			//check if subscribed, 
			if (!amSubscribed(channelToJoin)){
			//then subscribe to channel if not:

				std::cerr << myIP <<":"<<myPort<< " s2s adding channel " << channelToJoin <<std::endl;
 				struct channelInfo newChannel;
 				newChannel.myChannelName = channelToJoin;
 				mySubscribedChannels.push_back(newChannel);
				


			
				//for each neighbor, send join request
			
				sendS2Sjoin(channelToJoin,myIP,myPort);
			/*	for (std::vector<serverInfo>::iterator iter = serverList.begin(); iter != serverList.end(); ++iter) {
	 				//std::cerr << "neighbor..." << std::endl;
	 				struct sockaddr_in remoteAddress;
					remoteAddress.sin_family = AF_INET;
					remoteAddress.sin_port = htons((*iter).myPort);
					remoteAddress.sin_addr.s_addr = inet_addr((*iter).myIPAddress.c_str());\
//					std::cerr<< " sending mail to "<<(*iter).myIPAddress << " on port "<< (*iter).myPort<<std::endl;
					std::cerr << myIP <<":"<<myPort <<" "<<(*iter).myIPAddress<<":"<<(*iter).myPort<< " send S2S Join " <<channelToJoin<<std::endl;					

	 				if (sendto(mySocket, (char*)myBuffer, s2sJoinLeaveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
						perror("server sending s2s join request to multiple servers");
						exit(EXIT_FAILURE);
					}
	 			}*/
	 		}//else amSubscribed - do nothing

		}
		else if (identifier == REQ_S2S_LEAVE && bytesRecvd==s2sJoinLeaveSize){

			std::cerr << "received s2s leave request" << std::endl;

			


		}
		else if (identifier == REQ_S2S_SAY && bytesRecvd==s2sSaySize){
			std::cerr << "received s2s say request" << std::endl;


			struct request_s2s_say* incoming_request_s2s_say;
			incoming_request_s2s_say = (struct request_s2s_say*)myBuffer;
			std::string channel = std::string(incoming_request_s2s_say->req_channel);
			std::string userName = std::string(incoming_request_s2s_say->req_username);
			std::string message = std::string(incoming_request_s2s_say->req_username);

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
			handleRequest(myBuffer,bytesRecvd,inet_ntoa(remoteAddress.sin_addr),htons(remoteAddress.sin_port));	
			checkPurge(purgeTime);
		}		
	}
}
server::server(char* domain, char* port){
	myIP = std::string(domain);
	myPort = std::atoi(port);
	addressSize = sizeof(myAddress);
	bytesRecvd=0;
	mySocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (mySocket<0) {
		perror ("socket() failed");
		exit(EXIT_FAILURE);
	}
	//struct channelInfo newChannel;
	//newChannel.myChannelName = "Common";
	//mySubscribedChannels.push_back(newChannel);
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = inet_addr(domain);//htonl(INADDR_ANY);//inet_addr("128.223.4.39");//
	myAddress.sin_port = htons(std::atoi(port));
	if (bind(mySocket, (struct sockaddr *)&myAddress, sizeof(myAddress)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	//serve();
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