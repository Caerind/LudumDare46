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
		mShape.setRadius(50.0f);
		mShape.setPosition(512.f, 364.f);
		mShape.setFillColor(sf::Color::Green);

		GameSingleton::mClient.SetServerAddress(DefaultServerAddress);
		GameSingleton::mClient.SetServerPort(DefaultServerPort);
		GameSingleton::mClient.Start();

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
					sf::Packet pongPacket;
					pongPacket << static_cast<en::U8>(ClientPacketID::Pong);
					GameSingleton::mClient.SendPacket(pongPacket);
				} break;
				case ServerPacketID::Pong:
				{
					// Don't care now
				} break;
				case ServerPacketID::ClientAccept:
				{
					en::U32 clientID;
					packet >> clientID;
					GameSingleton::mClient.SetClientID(clientID);
					GameSingleton::mApplication->PushState<GameState>();
				} break;
				case ServerPacketID::ClientReject:
				{
					mShape.setFillColor(sf::Color::Red);
				} break;
				case ServerPacketID::ClientJoined:
				{
					// Don't care now
				} break;
				case ServerPacketID::ClientLeft:
				{
					// Don't care now
				} break;
				case ServerPacketID::Stopping:
				{
					mShape.setFillColor(sf::Color::Yellow);
				} break;
				case ServerPacketID::PlayerPosition:
				{
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