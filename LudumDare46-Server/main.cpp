#include <Enlivengine/System/Log.hpp>

#include "Server.hpp"

int main(int argc, char** argv)
{
	en::LogManager::GetInstance().Initialize();

	Server server;
	if (!server.Start(argc, argv))
	{
		return -1;
	}

	server.Run();

	return 0;
}