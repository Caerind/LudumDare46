#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/Application/Application.hpp>
#include <Enlivengine/Application/Window.hpp>
#include <Enlivengine/Graphics/View.hpp>

#include "MenuState.hpp"
#include "GameSingleton.hpp"

int main(int argc, char** argv)
{
	if (argc >= 1)
	{
		en::PathManager::GetInstance().SetExecutablePath(argv[0]);
	}

	en::Application::GetInstance().Initialize();

	en::Application& app = en::Application::GetInstance();
	GameSingleton::mApplication = &en::Application::GetInstance();
	app.GetWindow().create(sf::VideoMode(1024, 768), "LudumDare46", sf::Style::Titlebar | sf::Style::Close);
	app.GetWindow().getMainView().setCenter({ 512.0f, 384.0f });
	app.GetWindow().getMainView().setSize({ 1024.0f, 768.0f });
	en::PathManager::GetInstance().SetScreenshotPath("Screenshots/"); // TODO

	GameSingleton::ConnectWindowCloseSlot();

	// Cursor setup
	{
		en::Application::GetInstance().GetWindow().setCursor(en::Window::Cursor::None);
		GameSingleton::mCursor.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("cursor").Get());
		GameSingleton::mCursor.setTextureRect(sf::IntRect(0, 0, 32, 32));
	}

	en::AudioSystem::GetInstance().SetSoundVolume(0.1f);
	en::AudioSystem::GetInstance().SetMusicVolume(0.1f);

	GameSingleton::mIntroDone = false;
	GameSingleton::mMap.load();

	app.Start<MenuState>();

	return 0;
}