#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/Application/PathManager.hpp>
#include <Enlivengine/Application/Application.hpp>

#include "Server.hpp"

int main(int argc, char** argv)
{
	if (argc >= 1)
	{
		en::PathManager::GetInstance().SetExecutablePath(argv[0]);
	}

	en::Application::GetInstance().Initialize();

	Server server;
	if (!server.Start(argc, argv))
	{
		return -1;
	}

	server.Run();

	return 0;
}