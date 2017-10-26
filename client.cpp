#include "raw.h"
#include "shared.h"

class client{
	private:
		
		struct sockaddr_in remoteAddress;
		struct sockaddr_in myAddress;
		socklen_t addressSize;
		std::string userName;
		std::string remoteAddressString;
		int bytesRecvd;
		std::string myActiveChannel;
		std::vector<std::string> myChannels;

	public:
	int mySocket;
	int getServerResponse(bool nonblocking, char* replyBuffer){
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
	int handleServerResponse(char* replyBuffer,int bytesRecvd){
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
			return TXT_LIST;
		}
		else if (incoming_text->txt_type==TXT_WHO && bytesRecvd>=40){
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
		else if (incoming_text->txt_type==TXT_SAY/* &&  bytesRecvd==sayRequestSize*/){
			struct text_say* incoming_text_say;
			incoming_text_say = (struct text_say*)replyBuffer;
			std::string channel = incoming_text_say->txt_channel;
			std::string userName= incoming_text_say->txt_username;
			std::string message= incoming_text_say->txt_text;
			std::cerr<<"["<<channel<<"]["<<userName<<"]: "<<message<<std::endl;
			return TXT_SAY;
		}
	
		return 0;
	}

	void login(){
		truncate(userName,CHANNEL_MAX-1);
		struct request_login* my_request_login= new request_login;
		my_request_login->req_type = REQ_LOGIN;
		strcpy(my_request_login->req_username,userName.c_str());
		send((char*)my_request_login,loginSize,"sendto login");
		delete(my_request_login);
	}
	void logout(){
		struct request_logout* my_request_logout= new request_logout;
		my_request_logout->req_type = REQ_LOGOUT;
		send((char*)my_request_logout,logoutListSize,"sendto logout");
		close(mySocket);
		delete(my_request_logout);
	}

	void requestChannels(){
		char replyBuffer[BUFFERLENGTH];
		struct request_list* my_request_list= new request_list;
		my_request_list->req_type = REQ_LIST;
		send((char*)my_request_list,logoutListSize,"client requesting channels");
		int bytesRecvd = getServerResponse(false,replyBuffer);
		handleServerResponse(replyBuffer,bytesRecvd);
		delete(my_request_list);
	}
	void say(std::string textfield){
		truncate(textfield,SAY_MAX-1);
		struct request_say* my_request_say= new request_say;
		my_request_say->req_type = REQ_SAY;
		strcpy(my_request_say->req_channel,myActiveChannel.c_str());
		strcpy(my_request_say->req_text,textfield.c_str());
		send((char*)my_request_say,sayRequestSize,"sending a message to channel  (from client)");
	}
	void send(char* buf,int size,std::string error){
		if (sendto(mySocket, buf, size, 0, (struct sockaddr *)&remoteAddress, addressSize)==-1){
			perror(error.c_str());
			exit(-1);
		}
	}
	void switchChannel(std::string channel){
		truncate(channel,CHANNEL_MAX-1);
		struct request_switch* my_request_switch = new request_switch;
		my_request_switch->req_type = REQ_SWITCH;
		strcpy(my_request_switch->req_channel,channel.c_str());
		if (findStringPositionInVector(myChannels,channel)>-1)
			myActiveChannel = channel;
		send((char*)my_request_switch, joinLeaveWhoSize,"sendto request to join from client");
		delete(my_request_switch);
	}
	void join(std::string channel){
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
	void leave(std::string channel){
		truncate(channel,CHANNEL_MAX-1);
		struct request_leave* my_request_leave= new request_leave;
		my_request_leave->req_type = REQ_LEAVE;
		strcpy(my_request_leave->req_channel,channel.c_str());
		send((char*)my_request_leave,joinLeaveWhoSize,"sendto request to join from client");
		delete(my_request_leave);
	}	
	void who(std::string channel){
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


	client(){
		mySocket=0;
		addressSize=0;
		userName="";		
		bytesRecvd=0;
		remoteAddressString="";
		myActiveChannel="";
	}
	client(std::string user_name, std::string serverAddress){
		addressSize=sizeof(remoteAddress);
		userName=user_name;		
		bytesRecvd=0;
		remoteAddressString=serverAddress;
		mySocket=socket(AF_INET, SOCK_DGRAM, 0);
		if (mySocket==-1) {
			std::cerr << "socket created\n";
			
		}
		memset((char *) &remoteAddress, 0, sizeof(remoteAddress));
		remoteAddress.sin_family = AF_INET;
		remoteAddress.sin_port = htons(THEPORT);
		if (inet_aton(remoteAddressString.c_str(), &remoteAddress.sin_addr)==0) {
			fprintf(stderr, "inet_aton() failed\n");
			exit(1);
		}
		myActiveChannel="";

	}

	int parseCommand(std::string buffer){
		std::istringstream iss(buffer);
	    std::string command,parameter;
	    int running = 2;
	    if(iss >> command) {
	       if (command=="/exit"){
	           return 1;
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
	       		return 3;
	       }
	    }
	    return 42*running;
	    
	    
	}
};

int main (int argc, char *argv[]){
	client* thisClient = new client("Bobby Joeleine Smith4357093487509384750938475094387509348750439875430987435","127.0.0.1");//"128.223.4.39"
	thisClient->login();
	thisClient->join("Common");
	bool running = true;
	std::string command="";
	raw_mode();
	while (running){
		std::cerr<<">";
		char replyBuffer[BUFFERLENGTH];
		int err;
		fd_set readfds;
		FD_ZERO (&readfds);
		FD_SET (thisClient->mySocket, &readfds);
		FD_SET (STDIN_FILENO, &readfds);
		err = select (thisClient->mySocket + 1, &readfds, NULL, NULL, NULL);
		if (err < 0) perror ("select failed");
		else {
		        if (FD_ISSET (thisClient->mySocket, &readfds)){
		        	int bytesRecvd = thisClient->getServerResponse(true,replyBuffer);
					if (bytesRecvd>0){
						std::cerr<<'\b';
						int ret =0;
						ret = thisClient->handleServerResponse(replyBuffer,bytesRecvd);
						std::cerr << "returned : "<<(int)ret<<std::endl;//exit keeps returning wrong idk, time for bed
						//running = !running2;//???? function return was incorrect somehow?? switching fixed
					}
		        }
		        if (FD_ISSET (STDIN_FILENO, &readfds)){
		        	std::string buffer;
		        	getline(std::cin,buffer);
    				thisClient->parseCommand(buffer);

					FD_CLR(STDIN_FILENO,&readfds);
		        }
		}
	}
	thisClient->logout();
	delete(thisClient);
	return 0;
}