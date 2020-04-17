#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/Application/Application.hpp>
#include <Enlivengine/Application/Window.hpp>
#include <Enlivengine/Graphics/View.hpp>

#include "ConnectingState.hpp"

int main()
{
	en::Application::GetInstance().Initialize();

	en::Application& app = en::Application::GetInstance();
	app.GetWindow().create(sf::VideoMode(1024, 768), "LudumDare46", sf::Style::Titlebar | sf::Style::Close);
	app.GetWindow().getMainView().setCenter({ 512.0f, 384.0f });
	app.GetWindow().getMainView().setSize({ 1024.0f, 768.0f });
	en::PathManager::GetInstance().SetScreenshotPath("Screenshots/"); // TODO

	app.Start<ConnectingState>();

	return 0;
}