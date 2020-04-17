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
		LogInfo(en::LogChannel::All, 2, "Switching to ConnectingState%s", "");

		mShape.setRadius(50.0f);
		mShape.setPosition(0.0f, 0.0f);
		mShape.setFillColor(sf::Color::Green);

		GameSingleton::mClient.SetServerAddress(DefaultServerAddress);
		GameSingleton::mClient.SetServerPort(DefaultServerPort);
		GameSingleton::mClient.Start();
		GameSingleton::mLocalPosition = en::Vector2f::zero;
		GameSingleton::mInvalidLocalPosition = true;

		sf::Packet joinPacket;
		joinPacket << static_cast<en::U8>(ClientPacketID::Join);
		GameSingleton::mClient.SendPacket(joinPacket);
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
			if (!GameSingleton::mClient.IsConnected())
			{
				mWaitingTime += dt;
			}

			sf::Packet packet;
			while (GameSingleton::mClient.PollPacket(packet))
			{
				en::U8 packetIDRaw;
				packet >> packetIDRaw;
				assert(packetIDRaw < static_cast<en::U8>(ServerPacketID::Count));

				LogInfo(en::LogChannel::All, 2, "Received PacketID (%d)", packetIDRaw);
				const ServerPacketID packetID = static_cast<ServerPacketID>(packetIDRaw);
				switch (packetID)
				{
				case ServerPacketID::Ping:
				{
					LogInfo(en::LogChannel::All, 5, "Ping%s", "");
					sf::Packet pongPacket;
					pongPacket << static_cast<en::U8>(ClientPacketID::Pong);
					GameSingleton::mClient.SendPacket(pongPacket);
				} break;
				case ServerPacketID::Pong:
				{
					LogInfo(en::LogChannel::All, 5, "Pong%s", "");
				} break;
				case ServerPacketID::ClientAccept:
				{
					en::U32 clientID;
					packet >> clientID;
					LogInfo(en::LogChannel::All, 5, "ClientAccepted, ClientID %d %d", clientID, en::U32_Max);
					GameSingleton::mClient.SetClientID(clientID);
					GameSingleton::mApplication->ClearStates();
					GameSingleton::mApplication->PushState<GameState>();
				} break;
				case ServerPacketID::ClientReject:
				{
					en::U32 rejectReason;
					packet >> rejectReason;
					LogInfo(en::LogChannel::All, 5, "ClientRejected %d", rejectReason);
					mShape.setFillColor(sf::Color::Red);
					GameSingleton::mApplication->ClearStates();
				} break;
				case ServerPacketID::ClientJoined:
				{
					en::U32 clientID;
					en::Vector2f position;
					packet >> clientID >> position.x >> position.y;
					LogInfo(en::LogChannel::All, 5, "SomeoneJoined, ClientID %d", clientID);
					if (clientID == GameSingleton::mClient.GetClientID())
					{
						GameSingleton::mLocalPosition = position;
						GameSingleton::mInvalidLocalPosition = false;
					}
				} break;
				case ServerPacketID::ClientLeft:
				{
					en::U32 clientID;
					packet >> clientID;
					LogInfo(en::LogChannel::All, 5, "SomeoneLeft, ClientID %d", clientID);
				} break;
				case ServerPacketID::Stopping:
				{
					LogInfo(en::LogChannel::All, 5, "ServerStopping%s", "");
					mShape.setFillColor(sf::Color::Yellow);
				} break;
				case ServerPacketID::PlayerPosition:
				{
					LogWarning(en::LogChannel::All, 5, "PlayerPosition%s", "");
					// Don't care now
				} break;
				default:
				{
					LogWarning(en::LogChannel::All, 6, "Unknown ClientPacketID %d received", packetIDRaw);
				} break;
				}
			}
		}

		return false;
	}

	void render(sf::RenderTarget& target)
	{
		ENLIVE_PROFILE_FUNCTION();
		target.draw(mShape);
	}

private:
	sf::CircleShape mShape;
	en::Time mWaitingTime;
};