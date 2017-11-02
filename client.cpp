#include "client.h"
void client::keepAlive(){
	struct request_keep_alive* my_request_keep_alive = new request_keep_alive;
	my_request_keep_alive->req_type = REQ_KEEP_ALIVE;
	send((char*)my_request_keep_alive,logoutListKeepAliveSize,"keep Alive");
	delete(my_request_keep_alive);
}
void client::checkKeepAlive(time_t &keepAliveTime){
	time_t checkTime = time(NULL);	
	double seconds = difftime(checkTime,keepAliveTime);//note:order matters here
	if (seconds>=clientKeepAlive){
		keepAlive();
		keepAliveTime = time(NULL);	
	}
}
int client::getServerResponse(bool nonblocking, char* replyBuffer){
	int flag = 0;
	if (nonblocking)
		flag = MSG_DONTWAIT;
	int bytesRecvd=0;
	bytesRecvd = recvfrom(mySocket, replyBuffer, BUFFERLENGTH, flag, (struct sockaddr *)&remoteAddress, &addressSize);
	if (bytesRecvd>0){
		return bytesRecvd;
	}
	else return 0;
}
void client::handleServerResponse(char* replyBuffer,int bytesRecvd){
	if (bytesRecvd>=BUFFERLENGTH){
		std::cerr << "*buffer overflow, ignoring request" << std::endl;

	}
	else{
		struct text* incoming_text;
		incoming_text = (struct text*)replyBuffer;
		if (incoming_text->txt_type==TXT_LIST &&  bytesRecvd>=32){
			struct text_list* incoming_text_list;
			incoming_text_list = (struct text_list*)replyBuffer;
			int channels = incoming_text_list->txt_nchannels;
			struct channel_info* txt_channels;
			txt_channels = (struct channel_info*)incoming_text_list->txt_channels;
			std::vector<std::string> listOfChannels;
			for (int x=0; x<channels; x++){
				listOfChannels.push_back(std::string(txt_channels[x].ch_channel));
			}
			std::cerr<<"Existing channels:"<<std::endl;
			for (std::vector<std::string>::iterator iter = listOfChannels.begin(); iter != listOfChannels.end(); ++iter) {
				std::cerr << " "<<*iter << std::endl;
			}
		}
		else if (incoming_text->txt_type==TXT_WHO /*&& bytesRecvd>=joinLeaveWhoSize*/){//don't know who size
			struct text_who* incoming_text_who;
			incoming_text_who = (struct text_who*)replyBuffer;
			int userNames = incoming_text_who->txt_nusernames;
			std::string channel = std::string(incoming_text_who->txt_channel);
			struct user_info* txt_users;
			txt_users = (struct user_info*)incoming_text_who->txt_users;
			std::vector<std::string> listOfUsers;
			for (int x=0; x<userNames; x++){
				listOfUsers.push_back(std::string(txt_users[x].us_username));
			}
			std::cerr<<"Users on channel "<<channel<<":"<<std::endl;
			for (std::vector<std::string>::iterator iter = listOfUsers.begin(); iter != listOfUsers.end(); ++iter) {
				std::cerr << " "<<*iter << std::endl;
			}
		}
		else if (incoming_text->txt_type==TXT_SAY &&  bytesRecvd==saySize){
			struct text_say* incoming_text_say;
			incoming_text_say = (struct text_say*)replyBuffer;
			std::string channel = incoming_text_say->txt_channel;
			std::string userName= incoming_text_say->txt_username;
			std::string message= incoming_text_say->txt_text;
			std::cerr<<"["<<channel<<"]["<<userName<<"]: "<<message<<std::endl;
		}
		else if (incoming_text->txt_type==TXT_ERROR && bytesRecvd>=errorSize){//packet was 132, expected 68?
			struct text_error* incoming_text_error;
			incoming_text_error = (struct text_error*)replyBuffer;
			std::string errorMessage= incoming_text_error->txt_error;
			std::cerr << errorMessage << std::endl;
		}
	}
}
void client::login(){
	truncate(myUserName,CHANNEL_MAX-1);
	struct request_login* my_request_login= new request_login;
	my_request_login->req_type = REQ_LOGIN;
	initBuffer((char*)my_request_login->req_username, USERNAME_MAX);
	strcpy(my_request_login->req_username,myUserName.c_str());
	send((char*)my_request_login,loginSize,"sendto login");
	delete(my_request_login);
}
void client::logout(){
	struct request_logout* my_request_logout= new request_logout;
	my_request_logout->req_type = REQ_LOGOUT;
	send((char*)my_request_logout,logoutListKeepAliveSize,"sendto logout");
	close(mySocket);
	delete(my_request_logout);
}
void client::requestChannels(){
	char replyBuffer[BUFFERLENGTH];
	struct request_list* my_request_list= new request_list;
	my_request_list->req_type = REQ_LIST;
	send((char*)my_request_list,logoutListKeepAliveSize,"client requesting channels");
	int bytesRecvd = getServerResponse(false,replyBuffer);
	handleServerResponse(replyBuffer,bytesRecvd);
	delete(my_request_list);
}
void client::say(std::string textfield){
	if (myActiveChannel!=""){
		truncate(textfield,SAY_MAX-1);
		struct request_say* my_request_say= new request_say;
		my_request_say->req_type = REQ_SAY;
		initBuffer(my_request_say->req_channel, CHANNEL_MAX);
		initBuffer(my_request_say->req_text, SAY_MAX);
		strcpy(my_request_say->req_channel,myActiveChannel.c_str());
		strcpy(my_request_say->req_text,textfield.c_str());
		send((char*)my_request_say,sayRequestSize,"sending a message to channel  (from client)");
		delete(my_request_say);
	}
	else{
		std::cerr<<"*error you are not in a channel"<<std::endl;
	}

}
void client::send(char* buf,int size,std::string error){
	setTimeout(mySocket,clientResponseWaitTime);//i set this timeout so client won't hang if no response, but can't figure out how to detct it, but doesn't crash client at least
	int sending=0;
	sending = sendto(mySocket, buf, size, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress));
	if (sending==-1){
		perror(error.c_str());
		cooked_mode();
		exit(EXIT_FAILURE);
	}
	
}
void client::switchChannel(std::string channel){
	if (findStringPositionInVector(myChannels,channel)>-1)
		myActiveChannel = channel;
	else
		std::cerr << "*error, you have not joined channel "<<channel << std::endl;
}
void client::join(std::string channel){
	truncate(channel,CHANNEL_MAX-1);
	if (findStringPositionInVector(myChannels,channel)>-1)
		std::cerr << "*error, you have already joined channel "<<channel << std::endl;
	else{
		struct request_join* my_request_join= new request_join;
		my_request_join->req_type = REQ_JOIN;
		initBuffer(my_request_join->req_channel, CHANNEL_MAX);
		
		strcpy(my_request_join->req_channel,channel.c_str());
		myActiveChannel = channel;
		if (findStringPositionInVector(myChannels,channel)==-1)
			myChannels.push_back(channel);
		send((char*)my_request_join, joinLeaveWhoSize,"sendto request to join from client");
		delete(my_request_join);
	}
}
void client::leave(std::string channel){
	truncate(channel,CHANNEL_MAX-1);
	int position = findStringPositionInVector(myChannels,channel);
	if (position>-1){
		struct request_leave* my_request_leave= new request_leave;
		my_request_leave->req_type = REQ_LEAVE;
		initBuffer(my_request_leave->req_channel, CHANNEL_MAX);	
		strcpy(my_request_leave->req_channel,channel.c_str());
		send((char*)my_request_leave,joinLeaveWhoSize,"sendto request to join from client");
		delete(my_request_leave);
		myChannels.erase(myChannels.begin()+position);
		if (myActiveChannel==channel && channel!="Common" && findStringPositionInVector(myChannels,"Common")>=0){
			myActiveChannel="Common";//default to common, if still subscribed
		}
		else{
			myActiveChannel="";
		}

	}
	else{
		std::cerr << "*error, you have not joined channel "<<channel << std::endl;		
	}
}	
void client::who(std::string channel){
	truncate(channel,CHANNEL_MAX-1);
	char replyBuffer[BUFFERLENGTH];
	struct request_who* my_request_who= new request_who;
	my_request_who->req_type = REQ_WHO;
	initBuffer(my_request_who->req_channel, CHANNEL_MAX);
	strcpy(my_request_who->req_channel,channel.c_str());
	send((char*)my_request_who,joinLeaveWhoSize,"requesting who in channel (from client)");
	int bytesRecvd = getServerResponse(false,replyBuffer);
	handleServerResponse(replyBuffer,bytesRecvd);
	delete(my_request_who);
}
client::client(){
	mySocket=0;
	addressSize=0;
	myUserName="";		
	bytesRecvd=0;
	remoteAddressString="";
	myActiveChannel="";
}
client::client(char* serverAddress, char* serverPort, char* userName){
	addressSize=sizeof(remoteAddress);
	myUserName=std::string(userName);		
	bytesRecvd=0;
	remoteAddressString=std::string(serverAddress);
	remotePort=std::atoi(serverPort);
	mySocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (mySocket==-1) {
		std::cerr << "socket created\n";
	}
	remoteAddress.sin_family = AF_INET;
	remoteAddress.sin_port = htons(remotePort);
	if (inet_aton(remoteAddressString.c_str(), &remoteAddress.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	myActiveChannel="";
}

bool client::parseCommand(std::string buffer){
	std::istringstream iss(buffer);
    std::string command,parameter;
    bool running = true;
    if(iss >> command) {
       if (command=="/exit"){
           running = false;
       }
       else if (command=="/join"){
       	   	if (iss>>parameter){
       	   		join(parameter);
       	   }
       }
       else if (command=="/leave"){
       	   if (iss>>parameter){
       	   		leave(parameter);
       	   }
       }
       else if (command=="/list"){
           requestChannels();
       }
       else if (command=="/who"){
       	   if (iss>>parameter){
       	   		who(parameter);
       	   }
       }
       else if (command=="/switch"){
       	   if (iss>>parameter){
       	   		switchChannel(parameter);
       	   }
       }
       else if (command[0]=='/'){
       		std::cerr << "*Unknown command" << std::endl;
       }
       else{
    		say(buffer);       		
       }
    }
    return running;
}

int main (int argc, char *argv[]){
	if (argc!=4){
		std::cerr<<"Usage: ./client server_socket server_port username"<<std::endl;
		exit(EXIT_FAILURE);
	}	
	client* thisClient = new client(argv[1],argv[2],argv[3]);
	thisClient->login();
	thisClient->join("Common");
	bool running = true;
	std::string command="";
	raw_mode();
	std::string buffer="";
	if (buffer==""){
			std::cerr<<">";
	}
	struct timeval* timeOut=new timeval;
	time_t keepAliveTime = time (NULL);
	timeOut->tv_sec = clientKeepAlive/12;
	timeOut->tv_usec = 0;
		
	while (running){
		char replyBuffer[BUFFERLENGTH];
		int err;
		fd_set readfds;
		FD_ZERO (&readfds);
		FD_SET (thisClient->mySocket, &readfds);
		FD_SET (STDIN_FILENO, &readfds);
		err = select (thisClient->mySocket + 1, &readfds, NULL, NULL, timeOut);
		if (err < 0) {
			perror ("select failed");
			cooked_mode();
			exit(EXIT_FAILURE);
		}
		else if (err==0){
			thisClient->checkKeepAlive(keepAliveTime);
		}
		else {
	        if (FD_ISSET (thisClient->mySocket, &readfds)){
	        	int bytesRecvd = thisClient->getServerResponse(false,replyBuffer);
				if (bytesRecvd>0){
					for (unsigned int x=0; x <= buffer.size(); x++){
						std::cerr<<'\b';
					}
					thisClient->handleServerResponse(replyBuffer,bytesRecvd);
					std::cerr<<'>'<<buffer;
					thisClient->checkKeepAlive(keepAliveTime);
				}
	        }
	        if (FD_ISSET (STDIN_FILENO, &readfds)){
	        	char c;
	        	c = fgetc(stdin);
	        	if (c=='\n'){//run the command
	        		std::cerr<<std::endl;
	        		running = thisClient->parseCommand(buffer);
	        		std::cerr<<'\b'<<'\b'<<'>';
	        		buffer="";
	        	}
	        	else if (c==127){//backspace
	        		if (buffer.length()>0){
	        			std::cerr<<'\b'<<' '<<'\b';
						buffer=buffer.substr(0,buffer.length()-1);
					}
					thisClient->checkKeepAlive(keepAliveTime);
				}
	        	else {//output a character
	        		buffer += c;
	        		std::cerr<<c;
	        		thisClient->checkKeepAlive(keepAliveTime);
	        	}
	        	FD_CLR(STDIN_FILENO,&readfds);
	        }
		}    	
	}
	cooked_mode();
	thisClient->logout();
	delete(thisClient);
	delete(timeOut);//*/
	return 0;
}