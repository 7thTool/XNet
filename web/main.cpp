#include "XNet"
using namespace XNet;

class XServer : public XServerT<XServer>
{

};

int main(int argc, char* argv[])
{
	XServer server;
	server.start(5, 5);
	//server.listen(std::atoi(argv[1]));
	getchar();
	server.stop();

	return 0;
}

