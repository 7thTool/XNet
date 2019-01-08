#include "XServer.hpp"
using namespace XNet;

class XServer 
: public XServerT<XServer,XServer>
, public XServerT<XServer,XServer>::Listener
{

};

int main(int argc, char* argv[])
{
	XServer server;
	server.start(5);
	//server.listen(std::atoi(argv[1]));
	getchar();
	server.stop();

	return 0;
}

