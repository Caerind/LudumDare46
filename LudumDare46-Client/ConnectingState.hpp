#pragma once

#include <Enlivengine/Application/Application.hpp>
#include <Enlivengine/Application/StateManager.hpp>

#include <SFML/Graphics/CircleShape.hpp>

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
		GameSingleton::mClient.SetServerAddress(DefaultServerAddress);
		GameSingleton::mClient.SetServerPort(DefaultServerPort);
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
				if (GameSingleton::mLastPacketTime > DefaultTimeout)
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
	}

private:
	en::Time mWaitingTime;
};