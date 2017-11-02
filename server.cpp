#include "server.h"
//make function template if have time
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
	std::cerr<< "sending error to user at ip "<<ip<< " on port "<< port<<std::endl;
	if (sendto(mySocket, (char*)my_text_error, saySize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
		perror("server sending error to  user");
		exit(EXIT_FAILURE);
	}
	delete(my_text_error);
}
void server::purgeUsers(){
	//std::cerr << "purgeUsers initiated "<<std::endl;
	time_t timeNow = time(NULL);
	for (unsigned int x=0; x<currentUsers.size(); x++){
	
		double seconds = difftime(timeNow,currentUsers[x].lastSeen);
		//std::cerr << "seconds for user "<<currentUsers[x].myUserName <<" : "<<seconds<<std::endl;
		if (seconds>=serverTimeout){
			std::cerr << "server logs " <<currentUsers[x++].myUserName <<" out during purge." << std::endl;
     		
			currentUsers.erase(currentUsers.begin()+x);
     		if (x+1<=currentUsers.size())//at least one more?  this deserves more testing, but seems to work
     			x--;
     		else
     			break;						
		}
	}
}
void server::sendMessage(std::string fromUser, std::string toChannel, std::string message){
	//could probably recast the buffer instead of remaking it..
	int channelSlot = findChannelInfoPositionInVector(channelList,toChannel);
	if (channelSlot>-1){
		//get list of users/ips/ports, create text say, send message
		std::vector<std::string> listOfUsers = channelList[channelSlot].myUsers;
		std::vector<std::string> listOfIPs;
		std::vector<int> listOfPorts;
		int size = listOfUsers.size();
		for(int x =0 ; x < size; x++){
			//if (x!=userSlot){//assuming send mail to sender as well
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
		initBuffer(my_text_say->txt_channel, CHANNEL_MAX);
		initBuffer(my_text_say->txt_text, SAY_MAX);
		initBuffer(my_text_say->txt_username, USERNAME_MAX);
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
					exit(EXIT_FAILURE);
				}
			}			
		}
		delete(my_text_say);
	}
}
void server::handleRequest(char* myBuffer,int bytesRecvd){
	if (bytesRecvd>=BUFFERLENGTH){
		std::cerr << "*buffer overflow, ignoring request" << std::endl;
	}
	else if (bytesRecvd >= logoutListKeepAliveSize) {
		struct request* incoming_request;
		incoming_request = (struct request*)myBuffer;
		request_t identifier = incoming_request->req_type;
		std::string remoteIPAddress=inet_ntoa(remoteAddress.sin_addr);
		int remotePort =htons(remoteAddress.sin_port);
		socklen_t remoteIPAddressSize = sizeof(remoteAddress);
		if (identifier == REQ_LOGIN && bytesRecvd==loginSize){//login
			struct request_login* incoming_login_request;
			incoming_login_request = (struct request_login*)myBuffer;
			std::string userName = std::string(incoming_login_request->req_username);
			std::cerr << "server: " << userName <<" logs in" << std::endl;
			bool addUser = true;
			if (currentUsers.size()>0) {
				for (std::vector<userInfo>::iterator iter = currentUsers.begin(); iter != currentUsers.end(); ++iter) {
     				if (userName.compare((*iter).myUserName) == 0&&remoteIPAddress.compare((*iter).myIPAddress) == 0&&remotePort==(*iter).myPort){
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
     			newUser.myChannels.push_back("Common");
     			newUser.myActiveChannel = "Common";
     			newUser.lastSeen = time (NULL);
     			currentUsers.push_back(newUser);
     		}			     	
		}
		else if (identifier == REQ_LOGOUT && bytesRecvd==logoutListKeepAliveSize){//logout
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
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
			if (userSlot>=0){//user found in macro collection
 				//std::cerr << "found user "<<currentUsers[userSlot].myUserName<<std::endl;
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
	     					sendError("user already in channel",remoteIPAddress,remotePort);
	     					break;
						}
					}
					if (!userFound){
						channelList[position].myUsers.push_back(userName);
						std::cerr << "server: " << userName <<" joins channel " << channelToJoin <<std::endl;
					}
 				}
				currentUsers[userSlot].myActiveChannel = channelToJoin;
				currentUsers[userSlot].lastSeen = time (NULL);
     		}

		}
		else if (identifier == REQ_LEAVE && bytesRecvd == joinLeaveWhoSize){//leave request
			struct request_leave* incoming_leave_request;
			incoming_leave_request = (struct request_leave*)myBuffer;
			std::string channelToLeave = std::string(incoming_leave_request->req_channel);
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
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
 				currentUsers[userSlot].lastSeen = time (NULL);	
 			}
		}
		else if (identifier == REQ_SAY && bytesRecvd==sayRequestSize){//say request
			struct request_say* incoming_request_say;
			incoming_request_say = (struct request_say*)myBuffer;
			std::string channelToAnnounce = std::string(incoming_request_say->req_channel);
			std::string textField = std::string(incoming_request_say->req_text);						
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
			if (userSlot>=0){//user found
 				std::string userName=currentUsers[userSlot].myUserName;
 				std::cerr << "say request received from "<< "userName: " <<userName << " for channel " << channelToAnnounce << std::endl;
     			sendMessage(userName, channelToAnnounce, textField);
			}
			currentUsers[userSlot].lastSeen = time (NULL);
		}
		else if (identifier == REQ_KEEP_ALIVE && bytesRecvd==logoutListKeepAliveSize){//keep alive
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
 			if (userSlot>=0){//user found
 				std::cerr <<"keep alive from "<< currentUsers[userSlot].myUserName << std::endl;
 				currentUsers[userSlot].lastSeen = time (NULL);
 			}
		}
		else if (identifier == REQ_LIST && bytesRecvd==logoutListKeepAliveSize){//list of channels
			int size = channelList.size();
			int reserveSize = sizeof(text_list)+sizeof(channel_info)*size-1;
			struct text_list* my_text_list = (text_list*)malloc(reserveSize);
			my_text_list->txt_type = TXT_LIST;
			my_text_list->txt_nchannels = channelList.size();
			for (unsigned int x=0; x<channelList.size(); x++){
				initBuffer(my_text_list->txt_channels[x].ch_channel, CHANNEL_MAX);
				strcpy(my_text_list->txt_channels[x].ch_channel,channelList[x].myChannelName.c_str());
			}
			if (sendto(mySocket, my_text_list, reserveSize, 0, (struct sockaddr *)&remoteAddress, remoteIPAddressSize)==-1){
				perror("server sending channel list error");
				exit(EXIT_FAILURE);
			}
			free(my_text_list->txt_channels);
			free(my_text_list);
			int userSlot = findUserSlot(remoteIPAddress,remotePort);
			if (userSlot>=0){//user found
 				currentUsers[userSlot].lastSeen = time (NULL);
 			}
		}
		else if (identifier == REQ_WHO && bytesRecvd==joinLeaveWhoSize){//list of people on certain channel
			struct request_who* incoming_request_who;
			incoming_request_who = (struct request_who*)myBuffer;
			std::string channelToQuery = std::string(incoming_request_who->req_channel);
			int userSlot = findUserSlot(remoteIPAddress,remotePort);;
			std::string userName = currentUsers[userSlot].myUserName;
 			currentUsers[userSlot].lastSeen = time (NULL);
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
				initBuffer(my_text_who->txt_channel, CHANNEL_MAX);
				strcpy(my_text_who->txt_channel,channelToQuery.c_str());
				for (unsigned int x=0; x<channelList[position].myUsers.size(); x++){
					initBuffer(my_text_who->txt_users[x].us_username, USERNAME_MAX);
					strcpy(my_text_who->txt_users[x].us_username,channelList[position].myUsers[x].c_str());
				}
				if (sendto(mySocket, my_text_who, reserveSize, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
					perror("server sending who is error");
					exit(EXIT_FAILURE);
				}
				free(my_text_who);

			}
		}
		
		

	}
}


void server::serve(){
	setTimeout(mySocket,serverTimeout);
	/*struct timeval timeOut;
	timeOut.tv_sec = 120;
	timeOut.tv_usec = 0;
	int error1 = setsockopt(mySocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeOut,sizeof(struct timeval));
	if (error1<0){
		std::cerr<<"setsock error";
		exit(EXIT_FAILURE);
	}*/
	time_t purgeTime = time (NULL);
	//bool valgrind = true;
	while(true/*valgrind*/) {
		int bytesRecvd = 0;
		char myBuffer[BUFFERLENGTH];
		initBuffer(myBuffer,BUFFERLENGTH);
		//std::cerr <<"bound IP address:" << myDomain << " waiting on Port: "<< myPort <<std::endl;
		//printf("waiting on port %d\n", port.c_str);
		bytesRecvd = recvfrom(mySocket, myBuffer, BUFFERLENGTH, 0, (struct sockaddr *)&remoteAddress, &addressSize);
		if (bytesRecvd<=0){//never going to happen unless all users stop pinging in
			time_t timer = time (NULL);
			std::cerr<<"timeout!";
			std::cerr << timer;
			//purgeUsers();
			//valgrind=false;
			//check users still valid
		}
		else if (bytesRecvd>0){
			//printf("received %d bytes\n", bytesRecvd);
			handleRequest(myBuffer,bytesRecvd);	
			time_t checkTime = time(NULL);	
			double seconds = difftime(purgeTime,checkTime);
			if (seconds>=serverTimeout){
				std::cerr<<"timeout!";
			//	purgeUsers();
				purgeTime = time(NULL);	
				//valgrind=false;
			}


		}		
	}
}
server::server(char* domain, char* port){
	myDomain = std::string(domain);
	myPort = std::string(port);
	addressSize = sizeof(myAddress);
	bytesRecvd=0;
	mySocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (mySocket<0) {
		perror ("socket() failed");
		exit(EXIT_FAILURE);
	}
	struct channelInfo newChannel;
	newChannel.myChannelName = "Common";
	channelList.push_back(newChannel);
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = inet_addr(domain);//htonl(INADDR_ANY);//inet_addr("128.223.4.39");//
	myAddress.sin_port = htons(std::atoi(port));
	if (bind(mySocket, (struct sockaddr *)&myAddress, sizeof(myAddress)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	serve();
}

int main (int argc, char *argv[]){
	if (argc!=3){
		std::cerr<<argv[1]<<"Usage: ./server domain_name port_num"<<std::endl;
		exit(EXIT_FAILURE);
	}
	server* myServer = new server(argv[1],argv[2]);
	sleep(1);
	delete(myServer);
	return 0;
}