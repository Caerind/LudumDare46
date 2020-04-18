#include "Server.hpp"

#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/System/Hash.hpp>
#include <Enlivengine/System/Time.hpp>
#include <Enlivengine/Math/Random.hpp>

#include <SFML/Network.hpp>
#include <vector>

#include <Common.hpp>
#include "Player.hpp"
#include "ServerSocket.hpp"

Server::Server()
	: mSocket()
	, mRunning(false)
	, mStepTime(en::Time::Zero)
	, mTickTime(en::Time::Zero)
	, mPlayers()
	, mUpdateAllPlayers(false)
{
}

bool Server::Start(int argc, char** argv)
{
	mSocket.SetSocketPort((argc >= 2) ? static_cast<en::U16>(std::atoi(argv[1])) : DefaultServerPort);
	if (!mSocket.Start())
	{
		return false;
	}

	mRunning = true;

	return true;
}

void Server::Stop()
{
	mRunning = false;

	mSocket.SetBlocking(true);
	sf::Packet stoppingPacket;
	stoppingPacket << static_cast<en::U8>(ServerPacketID::Stopping);
	SendToAllPlayers(stoppingPacket);

	mSocket.Stop();
}

bool Server::Run()
{
	const en::Time stepInterval = DefaultStepInterval;
	const en::Time tickInterval = DefaultTickInterval;
	const sf::Time sleepTime = DefaultSleepTime;
	en::Clock clock;
	while (IsRunning())
	{
		HandleIncomingPackets();

		const en::Time dt = clock.restart();
		mStepTime += dt;
		mTickTime += dt;

		while (mStepTime >= stepInterval)
		{
			UpdateLogic();
			mStepTime -= stepInterval;
		}
		while (mTickTime >= tickInterval)
		{
			Tick();
			mTickTime -= tickInterval;
		}

		sf::sleep(sleepTime);
	}
	return true;
}

bool Server::IsRunning() const
{
	return mRunning;
}

void Server::UpdateLogic()
{
	// Nothing yet
}

void Server::Tick()
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		if (mPlayers[i].positionChanged || mUpdateAllPlayers)
		{
			SendPlayerPositionPacket(mPlayers[i].clientID, mPlayers[i].position);
			mPlayers[i].positionChanged = false;
		}
	}
	mUpdateAllPlayers = false;
}

void Server::HandleIncomingPackets()
{
	sf::Packet receivedPacket;
	sf::IpAddress remoteAddress;
	en::U16 remotePort;
	while (mSocket.PollPacket(receivedPacket, remoteAddress, remotePort))
	{
		bool ignorePacket = false;

		en::U8 packetIDRaw;
		receivedPacket >> packetIDRaw;
		if (packetIDRaw >= static_cast<en::U8>(ClientPacketID::Count))
		{
			ignorePacket = true;
		}

		LogInfo(en::LogChannel::All, 2, "Received PacketID (%d) from %s:%d", packetIDRaw, remoteAddress.toString().c_str(), remotePort);
		const ClientPacketID packetID = static_cast<ClientPacketID>(packetIDRaw);
		switch (packetID)
		{
		case ClientPacketID::Ping:
		{
			LogInfo(en::LogChannel::All, 5, "Ping%s", "");
			SendPongPacket(remoteAddress, remotePort);
		} break;
		case ClientPacketID::Pong:
		{
			LogInfo(en::LogChannel::All, 5, "Pong%s", "");
		} break;
		case ClientPacketID::Join:
		{
			const en::U32 clientID = GenerateClientID(remoteAddress, remotePort);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			const bool slotAvailable = (mPlayers.size() < DefaultMaxPlayers);
			const bool blacklisted = IsBlacklisted(remoteAddress);
			if (slotAvailable && !blacklisted && playerIndex == -1)
			{
				LogInfo(en::LogChannel::All, 5, "Player joined ClientID %d from %s:%d", clientID, remoteAddress.toString().c_str(), remotePort);

				Player newPlayer;
				newPlayer.remoteAddress = remoteAddress;
				newPlayer.remotePort = remotePort;
				newPlayer.clientID = clientID;
				newPlayer.lastPacketTime = en::Time::Zero;
				newPlayer.position = { en::Random::get<en::F32>(0.0f, 1024.f - 100.0f), en::Random::get<en::F32>(0.0f, 768.0f - 100.0f) };
				newPlayer.positionChanged = true;

				mPlayers.push_back(newPlayer);

				SendConnectionAcceptedPacket(remoteAddress, remotePort, clientID, newPlayer.position);
				SendClientJoinedPacket(clientID, newPlayer.position);
				mUpdateAllPlayers = true; // To force resend the position/data of other players
			}
			else
			{
				if (!slotAvailable)
				{
					LogInfo(en::LogChannel::All, 5, "Player can't join because maximum reached %s:%d", remoteAddress.toString().c_str(), remotePort);
					SendConnectionRejectedPacket(remoteAddress, remotePort, RejectReason::TooManyPlayers);
				}
				else if (blacklisted)
				{
					LogInfo(en::LogChannel::All, 5, "Player can't join because banned %s:%d", remoteAddress.toString().c_str(), remotePort);
					SendConnectionRejectedPacket(remoteAddress, remotePort, RejectReason::Blacklisted);
				}
				else
				{
					// Might be :
					// - Already connected
					LogInfo(en::LogChannel::All, 5, "Player can't join for unknown reason %s:%d", remoteAddress.toString().c_str(), remotePort);
					SendConnectionRejectedPacket(remoteAddress, remotePort, RejectReason::UnknownError);
				}
			}

		} break;
		case ClientPacketID::Leave:
		{
			const en::U32 clientID = GetClientIDFromIpAddress(remoteAddress, remotePort);
			if (clientID != en::U32_Max)
			{
				LogInfo(en::LogChannel::All, 5, "Player left ClientID %d from %s:%d", clientID, remoteAddress.toString().c_str(), remotePort);
				SendClientLeftPacket(clientID);
			}
			else
			{
				LogInfo(en::LogChannel::All, 5, "Unknown player left from %s:%d", remoteAddress.toString().c_str(), remotePort);
				ignorePacket = true;
			}
		} break;

		case ClientPacketID::PlayerPosition:
		{
			en::U32 clientID;
			en::Vector2f position;
			receivedPacket >> clientID >> position.x >> position.y;
			if (IsClientIDAssignedToThisPlayer(clientID, remoteAddress, remotePort))
			{
				const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
				if (playerIndex >= 0)
				{
					mPlayers[playerIndex].position = position;
					mPlayers[playerIndex].positionChanged = true;
				}
				else
				{
					ignorePacket = true;
				}
			}
			else
			{
				ignorePacket = true;
			}
		} break;

		default:
		{
			LogWarning(en::LogChannel::All, 6, "Unknown ClientPacketID %d received from %s:%d", packetIDRaw, remoteAddress.toString().c_str(), remotePort);
			ignorePacket = true;
		} break;
		}

		if (ignorePacket)
		{
			LogWarning(en::LogChannel::All, 4, "Ignore packet %d from %s:%d", packetIDRaw, remoteAddress.toString().c_str(), remotePort);
		}
	}
}

en::U32 Server::GenerateClientID(const sf::IpAddress& remoteAddress, en::U16 remotePort) const
{
	return en::Hash::CombineHash(en::Hash::CRC32(remoteAddress.toString().c_str()), static_cast<en::U32>(remotePort));
}

bool Server::IsClientIDAssignedToThisPlayer(en::U32 clientID, const sf::IpAddress& remoteAddress, en::U16 remotePort) const
{
	return clientID == GenerateClientID(remoteAddress, remotePort);
}

en::U32 Server::GetClientIDFromIpAddress(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::I32* playerIndex) const
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		if (mPlayers[i].remotePort == remotePort && mPlayers[i].remoteAddress == remoteAddress)
		{
			if (playerIndex != nullptr)
			{
				*playerIndex = static_cast<en::I32>(i);
			}
			return mPlayers[i].clientID;
		}
	}
	if (playerIndex != nullptr)
	{
		*playerIndex = -1;
	}
	return en::U32_Max;
}

en::I32 Server::GetPlayerIndexFromClientID(en::U32 clientID) const
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

bool Server::IsBlacklisted(const sf::IpAddress& remoteAddress) const
{
	return false; // TODO : Blacklisted
}

void Server::SendToAllPlayers(sf::Packet& packet)
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		mSocket.SendPacket(packet, mPlayers[i].remoteAddress, mPlayers[i].remotePort);
	}
}

void Server::SendPingPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::Ping);
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendPongPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::Pong);
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendConnectionAcceptedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::U32 clientID, const en::Vector2f& position)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ConnectionAccepted);
		packet << clientID;
		packet << position.x;
		packet << position.y;
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendConnectionRejectedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, RejectReason reason)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ConnectionRejected);
		packet << static_cast<en::U32>(reason);
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendClientJoinedPacket(en::U32 clientID, const en::Vector2f& position)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ClientLeft);
		packet << clientID;
		packet << position.x;
		packet << position.y;
		SendToAllPlayers(packet);
	}
}

void Server::SendClientLeftPacket(en::U32 clientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ClientLeft);
		packet << clientID;
		SendToAllPlayers(packet);
	}
}

void Server::SendServerStopPacket()
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::Stopping);
		SendToAllPlayers(packet);
	}
}

void Server::SendPlayerPositionPacket(en::U32 clientID, const en::Vector2f& position)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::PlayerPosition);
		packet << clientID;
		packet << position.x;
		packet << position.y;
		SendToAllPlayers(packet);
	}
}
