#include "GameSingleton.hpp"

#include <Common.hpp>

en::Application* GameSingleton::mApplication;
ClientSocket GameSingleton::mClient;
en::Time GameSingleton::mLastPacketTime;
GameMap GameSingleton::mMap;
std::vector<Player> GameSingleton::mPlayers;
std::vector<Seed> GameSingleton::mSeeds;
std::vector<en::Vector2f> GameSingleton::mCancelSeeds;

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
				packet >> clientID;
				LogInfo(en::LogChannel::All, 5, "ConnectionAccepted, ClientID %d", clientID);
				mClient.SetClientID(clientID);
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
		} break;
		case ServerPacketID::ClientJoined:
		{
			if (!mClient.IsConnected())
			{
				LogError(en::LogChannel::All, 2, "ClientJoined but not connected%s", "");
				return;
			}
			en::U32 clientID;
			std::string nickname;
			Chicken chicken;
			packet >> clientID >> nickname >> chicken;
			LogInfo(en::LogChannel::All, 5, "ClientJoined, ClientID %d, Nickname %s", clientID, nickname.c_str());
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex == -1)
			{
				Player newPlayer;
				newPlayer.clientID = clientID;
				newPlayer.nickname = nickname;
				newPlayer.chicken = chicken;
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
		case ServerPacketID::UpdateChicken:
		{
			if (!mClient.IsConnected())
			{
				LogError(en::LogChannel::All, 2, "PlayerPosition but not connected%s", "");
				return;
			}
			en::U32 clientID;
			packet >> clientID;
			LogInfo(en::LogChannel::All, 5, "PlayerPosition, ClientID %d", clientID);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex >= 0)
			{
				packet >> mPlayers[playerIndex].chicken;
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Unknown player moved %d", packetIDRaw);
			}
		} break;
		case ServerPacketID::CancelSeed:
		{
			en::Vector2f position;
			packet >> position.x >> position.y;
			LogInfo(en::LogChannel::All, 4, "Cancel seed %f %f", position.x, position.y);
			mCancelSeeds.push_back(position);
		} break;
		case ServerPacketID::AddSeed:
		{
			Seed seed;
			packet >> seed.seedID >> seed.position.x >> seed.position.y;
			LogInfo(en::LogChannel::All, 4, "New seed %d %f %f", seed.seedID, seed.position.x, seed.position.y);
			mSeeds.push_back(seed);
		} break;
		case ServerPacketID::RemoveSeed:
		{
			en::U32 seedID;
			packet >> seedID;
			LogInfo(en::LogChannel::All, 4, "Remove seed %d", seedID);
			bool removed = false;
			en::U32 size = static_cast<en::U32>(mSeeds.size());
			for (en::U32 i = 0; i < size && !removed; )
			{
				if (mSeeds[i].seedID == seedID)
				{
					mSeeds.erase(mSeeds.begin() + i);
					size--;
					removed = true;
				}
				else
				{
					++i;
				}
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

void GameSingleton::SendDropSeedPacket(const en::Vector2f& position)
{
	if (mClient.IsRunning() && mClient.IsConnected())
	{
		sf::Packet positionPacket;
		positionPacket << static_cast<en::U8>(ClientPacketID::DropSeed);
		positionPacket << mClient.GetClientID();
		positionPacket << position.x;
		positionPacket << position.y;
		mClient.SendPacket(positionPacket);
	}
}
