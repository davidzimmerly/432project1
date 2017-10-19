struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myCurrentChannel;
	std::string myIPAddress;
};

union intOrBytes
{
	unsigned int integer;
	unsigned char byte[4];
};

void initBuffer(unsigned char* buf, unsigned int length){
	for (unsigned int x=0; x<length ; x++){
		buf[x]='\0';
	}
}
