#include "Server.hpp"

int main(int argc, char** argv)
{
	Server server;
	if (!server.Start(argc, argv))
	{
		return -1;
	}

	server.Run();

	return 0;
}