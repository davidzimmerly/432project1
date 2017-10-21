const uint8_t loginSize = 36;
const uint8_t logoutSize = 4;
const uint8_t joinSize = 36;
const uint8_t requestChannelSize = 4;


struct userInfo
{
	std::string myUserName;
	std::vector<std::string> myChannels;
	std::string myActiveChannel;
	std::string myIPAddress;
};

union intOrBytes
{
	uint32_t integer;
	unsigned char byte[4];
};

void initBuffer(unsigned char* buf, unsigned int length){
	for (uint32_t x=0; x<length ; x++){
		buf[x]='\0';
	}
}
