#include "GameState.hpp"

#include "ConnectingState.hpp"
#include "MenuState.hpp"

ConnectingState::ConnectingState(en::StateManager& manager)
	: en::State(manager)
	, mWaitingTime(en::Time::Zero)
{
	if (GameSingleton::mClient.IsRunning() || GameSingleton::mClient.IsConnected())
	{
		GameSingleton::mClient.SetClientID(en::U32_Max);
		GameSingleton::mClient.Stop();
	}

	sf::IpAddress remoteAddress = DefaultServerAddress;
	en::U16 remotePort = DefaultServerPort;
	std::ifstream serverConfigFile("server.txt");
	if (serverConfigFile)
	{
		std::string address;
		serverConfigFile >> address;
		serverConfigFile >> remotePort;
		serverConfigFile.close();
		remoteAddress = sf::IpAddress(address);
	}

	GameSingleton::mClient.SetServerAddress(remoteAddress);
	GameSingleton::mClient.SetServerPort(remotePort);
	GameSingleton::mClient.Start();

	mWaitingTime = en::seconds(2.0f);
}

bool ConnectingState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	return false;
}

bool ConnectingState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();

	if (GameSingleton::mClient.IsRunning())
	{
		const bool wasConnected = GameSingleton::mClient.IsConnected();

		if (mWaitingTime >= en::seconds(0.7f))
		{
			GameSingleton::SendJoinPacket();
			mWaitingTime = en::Time::Zero;
			LogInfo(en::LogChannel::All, 3, "Try to connect", "");
			static en::U32 i = 0;
			i++;
			i %= 3;
			if (i == 0)
			{
				mTextString = "Connecting.";
			}
			else if (i == 1)
			{
				mTextString = "Connecting..";
			}
			else if (i == 2)
			{
				mTextString = "Connecting...";
			}
		}
		if (!GameSingleton::mClient.IsConnected())
		{
			mWaitingTime += dt;
		}

		{
			ENLIVE_PROFILE_FUNCTION();
			GameSingleton::HandleIncomingPackets();
			if (GameSingleton::HasTimeout(dt))
			{
				clearStates();
				pushState<MenuState>();
			}
		}

		if (!wasConnected && GameSingleton::mClient.IsConnected())
		{
			clearStates();
			pushState<GameState>();
		}
	}
	else
	{
		clearStates();
		pushState<MenuState>();
		LogWarning(en::LogChannel::All, 5, "Invalid socket%s", "");
	}

	return false;
}

void ConnectingState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();

	static bool init = false;
	static sf::Sprite screen;
	static sf::Text text;
	if (!init)
	{
		screen.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("screen_connect").Get());
		text.setFont(en::ResourceManager::GetInstance().Get<en::Font>("MainFont").Get());
		text.setCharacterSize(24);
		text.setPosition(1024.0f - 400.0f, 768.0f - 100.0f);
		text.setFillColor(sf::Color::White);
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(1.0f);
		init = true;
	}
	text.setString(mTextString);
	target.draw(text);
	target.draw(screen);

	GameSingleton::mCursor.setPosition(en::toSF(GameSingleton::mApplication->GetWindow().getCursorPosition()));
	GameSingleton::mCursor.setTextureRect(sf::IntRect(0, 0, 32, 32));
	target.draw(GameSingleton::mCursor);
}
