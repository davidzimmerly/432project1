#include "client.h"

void client::keepAlive(){
	struct request_keep_alive* my_request_keep_alive = new request_keep_alive;
	my_request_keep_alive->req_type = REQ_KEEP_ALIVE;
	send((char*)my_request_keep_alive,logoutListKeepAliveSize,"keep Alive");
	delete(my_request_keep_alive);
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
	//assuming you have checked size of response first so it matches one of the types (need function to check this)
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
		//std::cerr <<"received " << bytesRecvd<<" error message: "<<errorMessage<<std::endl;
		std::cerr << errorMessage << std::endl;
		cooked_mode();
		exit(EXIT_FAILURE);

	}
}

void client::login(){
	truncate(myUserName,CHANNEL_MAX-1);
	struct request_login* my_request_login= new request_login;
	my_request_login->req_type = REQ_LOGIN;
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
	truncate(textfield,SAY_MAX-1);
	struct request_say* my_request_say= new request_say;
	my_request_say->req_type = REQ_SAY;
	strcpy(my_request_say->req_channel,myActiveChannel.c_str());
	strcpy(my_request_say->req_text,textfield.c_str());
	send((char*)my_request_say,sayRequestSize,"sending a message to channel  (from client)");
	delete(my_request_say);
}
void client::send(char* buf,int size,std::string error){
	if (sendto(mySocket, buf, size, 0, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress))==-1){
		perror(error.c_str());
		exit(-1);
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
	struct request_join* my_request_join= new request_join;
	my_request_join->req_type = REQ_JOIN;
	strcpy(my_request_join->req_channel,channel.c_str());
	myActiveChannel = channel;
	if (findStringPositionInVector(myChannels,channel)==-1)
		myChannels.push_back(channel);
	send((char*)my_request_join, joinLeaveWhoSize,"sendto request to join from client");
	delete(my_request_join);
}
void client::leave(std::string channel){
	truncate(channel,CHANNEL_MAX-1);
	struct request_leave* my_request_leave= new request_leave;
	my_request_leave->req_type = REQ_LEAVE;
	strcpy(my_request_leave->req_channel,channel.c_str());
	send((char*)my_request_leave,joinLeaveWhoSize,"sendto request to join from client");
	delete(my_request_leave);
}	
void client::who(std::string channel){
	truncate(channel,CHANNEL_MAX-1);
	char replyBuffer[BUFFERLENGTH];
	struct request_who* my_request_who= new request_who;
	my_request_who->req_type = REQ_WHO;
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