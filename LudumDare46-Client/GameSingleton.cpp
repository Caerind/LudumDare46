#include "GameSingleton.hpp"

#include <Common.hpp>

en::Application* GameSingleton::mApplication;
ClientSocket GameSingleton::mClient;
en::Time GameSingleton::mLastPacketTime;
en::Vector2f GameSingleton::mLocalPosition;
GameMap GameSingleton::mMap;
en::View GameSingleton::mView;
std::vector<Player> GameSingleton::mPlayers;

void GameSingleton::HandleIncomingPackets()
{
	if (!mClient.IsRunning())
	{
		return;
	}

	sf::Packet packet;
	while (mClient.PollPacket(packet))
	{
		mLastPacketTime = en::Time::Zero;

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
			SendPongPacket();
		} break;
		case ServerPacketID::Pong:
		{
			LogInfo(en::LogChannel::All, 5, "Pong%s", "");
		} break;
		case ServerPacketID::ConnectionAccepted:
		{
			if (!mClient.IsConnected())
			{
				en::U32 clientID;
				en::Vector2f position;
				packet >> clientID >> position.x >> position.y;
				LogInfo(en::LogChannel::All, 5, "ConnectionAccepted, ClientID %d at %f %f", clientID, position.x, position.y);
				mClient.SetClientID(clientID);
				mLocalPosition = position;
			}
			else
			{
				LogError(en::LogChannel::All, 9, "Already connected %d", mClient.GetClientID());
			}
		} break;
		case ServerPacketID::ConnectionRejected:
		{
			en::U32 rejectReason;
			packet >> rejectReason;
			LogInfo(en::LogChannel::All, 5, "ConnectionRejected %d", rejectReason);
			mClient.SetClientID(en::U32_Max);
			mClient.Stop();
			mLocalPosition = {0.0f, 0.0f};
		} break;
		case ServerPacketID::ClientJoined:
		{
			if (!mClient.IsConnected())
			{
				LogError(en::LogChannel::All, 2, "ClientJoined but not connected%s", "");
				return;
			}
			en::U32 clientID;
			en::Vector2f position;
			packet >> clientID >> position.x >> position.y;
			LogInfo(en::LogChannel::All, 5, "ClientJoined, ClientID %d at %f %f", clientID, position.x, position.y);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex == -1)
			{
				Player newPlayer;
				newPlayer.shape.setSize({ 100.0f, 100.0f });
				newPlayer.shape.setFillColor(sf::Color::Green);
				newPlayer.shape.setPosition(en::toSF(position));
				newPlayer.clientID = clientID;
				mPlayers.push_back(newPlayer);
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Client already joined%s", "");
			}
		} break;
		case ServerPacketID::ClientLeft:
		{
			if (!mClient.IsConnected())
			{
				LogError(en::LogChannel::All, 2, "ClientJoined but not connected%s", "");
				return;
			}
			en::U32 clientID;
			packet >> clientID;
			LogInfo(en::LogChannel::All, 5, "ClientLeft, ClientID %d", clientID);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex >= 0)
			{
				mPlayers.erase(mPlayers.begin() + static_cast<en::U32>(playerIndex));
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Client already left or not logged%s", "");
			}
		} break;
		case ServerPacketID::Stopping:
		{
			LogInfo(en::LogChannel::All, 5, "ServerStopping%s", "");
			mClient.SetClientID(en::U32_Max);
			mClient.Stop();
		} break;
		case ServerPacketID::PlayerPosition:
		{
			if (!mClient.IsConnected())
			{
				LogError(en::LogChannel::All, 2, "PlayerPosition but not connected%s", "");
				return;
			}
			en::U32 clientID;
			en::Vector2f position;
			packet >> clientID >> position.x >> position.y;
			LogInfo(en::LogChannel::All, 5, "PlayerPosition, ClientID %d at %f %f", clientID, position.x, position.y);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex >= 0)
			{
				mPlayers[playerIndex].shape.setPosition(en::toSF(position));
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Unknown player moved %d", packetIDRaw);
			}
		} break;
		default:
		{
			LogWarning(en::LogChannel::All, 6, "Unknown ServerPacketID %d received", packetIDRaw);
		} break;
		}
	}
}

en::I32 GameSingleton::GetPlayerIndexFromClientID(en::U32 clientID)
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		if (mPlayers[i].clientID == clientID)
		{
			return static_cast<en::I32>(i);
		}
	}
	return -1;
}

void GameSingleton::SendPingPacket()
{
	sf::Packet pingPacket;
	pingPacket << static_cast<en::U8>(ClientPacketID::Ping);
	mClient.SendPacket(pingPacket);
}

void GameSingleton::SendPongPacket()
{
	sf::Packet pongPacket;
	pongPacket << static_cast<en::U8>(ClientPacketID::Pong);
	mClient.SendPacket(pongPacket);
}

void GameSingleton::SendJoinPacket()
{
	if (mClient.IsRunning() && !mClient.IsConnected())
	{
		sf::Packet joinPacket;
		joinPacket << static_cast<en::U8>(ClientPacketID::Join);
		mClient.SendPacket(joinPacket);
	}
}

void GameSingleton::SendLeavePacket()
{
	if (mClient.IsRunning() && mClient.IsConnected())
	{
		sf::Packet leavePacket;
		leavePacket << static_cast<en::U8>(ClientPacketID::Leave);
		leavePacket << mClient.GetClientID();
		mClient.SetBlocking(true);
		mClient.SendPacket(leavePacket);
		mClient.SetBlocking(false);
	}
}

void GameSingleton::SendPositionPacket(const en::Vector2f& position)
{
	if (mClient.IsRunning() && mClient.IsConnected())
	{
		sf::Packet positionPacket;
		positionPacket << static_cast<en::U8>(ClientPacketID::PlayerPosition);
		positionPacket << mClient.GetClientID();
		positionPacket << position.x;
		positionPacket << position.y;
		mClient.SendPacket(positionPacket);
	}
}
