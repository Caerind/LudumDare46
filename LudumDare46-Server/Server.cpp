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
			UpdateLogic(stepInterval);
			mStepTime -= stepInterval;
		}
		while (mTickTime >= tickInterval)
		{
			Tick(tickInterval);
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

void Server::UpdateLogic(en::Time dt)
{
	en::U32 seedSize = static_cast<en::U32>(mSeeds.size());
	en::F32 dtSeconds = dt.asSeconds();

	const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		mPlayers[i].lastSeedTime += dt;

		UpdatePlayerMovement(dtSeconds, mPlayers[i]);
	}

	for (en::U32 i = 0; i < seedSize;)
	{
		mSeeds[i].addTime += dt;
		if (mSeeds[i].addTime >= DefaultSeedLifetime)
		{
			SendRemoveSeedPacket(mSeeds[i].seedID);
			mSeeds.erase(mSeeds.begin() + i);
			seedSize--;
		}
		else
		{
			i++;
		}
	}
}

void Server::Tick(en::Time dt)
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		SendUpdateChickenPacket(mPlayers[i].clientID, mPlayers[i].chicken);
		mPlayers[i].lastPacketTime += dt;
	}
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
				newPlayer.nickname = "Player" + std::to_string(clientID); // TODO : nickname
				newPlayer.chicken.position = { en::Random::get<en::F32>(0.0f, 1024.f - 100.0f), en::Random::get<en::F32>(0.0f, 768.0f - 100.0f) };
				newPlayer.chicken.rotation = 0.0f;
				newPlayer.chicken.itemID = 0;
				newPlayer.chicken.life = DefaultChickenLife;
				newPlayer.chicken.speed = DefaultChickenSpeed;
				newPlayer.chicken.attack = DefaultChickenAttack;
				newPlayer.state = PlayingState::Playing;
				newPlayer.lastSeedTime = en::Time::Zero;

				mPlayers.push_back(newPlayer);

				SendConnectionAcceptedPacket(remoteAddress, remotePort, clientID);

				SendClientJoinedPacket(clientID, newPlayer.nickname, newPlayer.chicken);
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

		case ClientPacketID::DropSeed:
		{
			en::U32 clientID;
			en::Vector2f position;
			receivedPacket >> clientID >> position.x >> position.y;
			if (IsClientIDAssignedToThisPlayer(clientID, remoteAddress, remotePort))
			{
				const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
				if (playerIndex >= 0)
				{
					if (mPlayers[playerIndex].lastSeedTime >= DefaultSeedInterval)
					{
						LogInfo(en::LogChannel::All, 2, "Player %d dropped seed at %f %f", mPlayers[playerIndex].clientID, position.x, position.y);
						mPlayers[playerIndex].lastSeedTime = en::Time::Zero;
						AddNewSeed(position);
					}
					else
					{
						LogInfo(en::LogChannel::All, 2, "Player %d seed cooldown", mPlayers[playerIndex].clientID);
						SendCancelSeedPacket(remoteAddress, remotePort, position);
					}
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

void Server::UpdatePlayerMovement(en::F32 dtSeconds, Player& player)
{
	// Select base seed
	en::I32 bestSeedIndex = -1;
	en::F32 bestSeedDistanceSqr = 999999.0f;
	const en::U32 seedSize = static_cast<en::U32>(mSeeds.size());
	for (en::U32 i = 0; i < seedSize; ++i)
	{
		const en::Vector2f delta = mSeeds[i].position - player.chicken.position;
		const en::F32 distanceSqr = delta.getSquaredLength();
		if (distanceSqr < bestSeedDistanceSqr)
		{
			bestSeedDistanceSqr = distanceSqr;
			bestSeedIndex = static_cast<en::I32>(i);
		}
	}

	// Movement based on seed
	en::Vector2f movement(0.0f, 0.0f);
	if (bestSeedIndex >= 0)
	{
		const Seed& seed = mSeeds[bestSeedIndex];
		const en::Vector2f delta = seed.position - player.chicken.position;
		const en::F32 distanceSqr = delta.getSquaredLength();
		if (DefaultSeedTooCloseDistanceSqr < distanceSqr && distanceSqr < DefaultSeedImpactDistanceSqr)
		{
			const en::F32 factor = (DefaultSeedImpactDistanceSqr - distanceSqr) / DefaultSeedImpactDistanceSqr;
			movement += delta * factor;
		}
	}

	// Avoidance
	const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		const Player& otherPlayer = mPlayers[i];
		if (otherPlayer.clientID != player.clientID)
		{
			const en::Vector2f delta = player.chicken.position - otherPlayer.chicken.position;
			const en::F32 distanceSqr = delta.getSquaredLength();
			if (0.01f < distanceSqr && distanceSqr < DefaultChickenAvoidanceMinDistanceSqr)
			{
				const en::F32 factor = (DefaultChickenAvoidanceMinDistanceSqr - distanceSqr) / DefaultChickenAvoidanceMinDistanceSqr;
				movement += delta * factor;
			}
			else if (distanceSqr <= 0.01f)
			{
				player.chicken.position += { 10.0f, 10.0f };
			}
		}
	}

	// Update the movement
	if (movement.getSquaredLength() > 0.0f)
	{
		movement.normalize();
		movement *= dtSeconds * player.chicken.speed;
		player.chicken.position += movement;
		if (en::Math::Equals(movement.x, 0.0f))
		{ 
			// Cheating a bit
			if (movement.x > 0.0f)
			{
				movement.x += 0.01f;
			}
			else
			{
				movement.x -= 0.01f;
			}
		}
		player.chicken.rotation = movement.getPolarAngle();
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

void Server::AddNewSeed(const en::Vector2f& position)
{
	static en::U32 seedIDGenerator = 1;
	Seed seed;
	seed.seedID = seedIDGenerator++;
	seed.position = position;
	seed.addTime = en::Time::Zero;
	mSeeds.push_back(seed);
	SendAddSeedPacket(seed);
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

void Server::SendConnectionAcceptedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::U32 clientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ConnectionAccepted);
		packet << clientID;
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

void Server::SendClientJoinedPacket(en::U32 clientID, const std::string& nickname, const Chicken& chicken)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ClientJoined);
		packet << clientID;
		packet << nickname;
		packet << chicken;
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
		mSocket.SetBlocking(true);
		SendToAllPlayers(packet);
		mSocket.SetBlocking(false);
	}
}

void Server::SendUpdateChickenPacket(en::U32 clientID, const Chicken& chicken)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::UpdateChicken);
		packet << clientID;
		packet << chicken;
		SendToAllPlayers(packet);
	}
}

void Server::SendCancelSeedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, const en::Vector2f& position)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::CancelSeed);
		packet << position.x;
		packet << position.y;
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendAddSeedPacket(const Seed& seed)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::AddSeed);
		packet << seed.seedID;
		packet << seed.position.x;
		packet << seed.position.y;
		SendToAllPlayers(packet);
	}
}

void Server::SendRemoveSeedPacket(en::U32 seedID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::RemoveSeed);
		packet << seedID;
		SendToAllPlayers(packet);
	}
}
