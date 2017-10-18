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
