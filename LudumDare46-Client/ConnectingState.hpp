#pragma once

#include <Enlivengine/Application/Application.hpp>
#include <Enlivengine/Application/StateManager.hpp>

#include <fstream>

#include "GameSingleton.hpp"
#include "GameState.hpp"

class ConnectingState : public en::State
{
public:
	ConnectingState(en::StateManager& manager)
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
	
	bool handleEvent(const sf::Event& event)
	{
		ENLIVE_PROFILE_FUNCTION();

		return false;
	}

	bool update(en::Time dt)
	{
		ENLIVE_PROFILE_FUNCTION();
		
		if (GameSingleton::mClient.IsRunning())
		{
			const bool wasConnected = GameSingleton::mClient.IsConnected();

			if (mWaitingTime >= en::seconds(1.0f))
			{
				GameSingleton::SendJoinPacket();
				mWaitingTime = en::Time::Zero;
				LogInfo(en::LogChannel::All, 3, "Try to connect", "");
			}
			if (!GameSingleton::mClient.IsConnected())
			{
				mWaitingTime += dt;
			}

			{
				ENLIVE_PROFILE_FUNCTION();
				GameSingleton::HandleIncomingPackets();
			}
			if (GameSingleton::mClient.IsConnected())
			{
				GameSingleton::mLastPacketTime += dt;
				if (GameSingleton::mLastPacketTime > DefaultClientTimeout)
				{
					GameSingleton::mClient.SetClientID(en::U32_Max);
					GameSingleton::mClient.Stop();
					clearStates();
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
			LogWarning(en::LogChannel::All, 5, "Invalid socket%s", "");
		}

		return false;
	}

	void render(sf::RenderTarget& target)
	{
		ENLIVE_PROFILE_FUNCTION();

		GameSingleton::mCursor.setPosition(en::toSF(GameSingleton::mApplication->GetWindow().getCursorPosition()));
		GameSingleton::mCursor.setTextureRect(sf::IntRect(0, 0, 32, 32));
		target.draw(GameSingleton::mCursor);
	}

private:
	en::Time mWaitingTime;
};