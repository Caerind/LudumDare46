#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/System/Hash.hpp>
#include <Enlivengine/System/Time.hpp>
#include <Enlivengine/Math/Random.hpp>

#include <SFML/Network.hpp>
#include <vector>

#include <Common.hpp>
#include "Player.hpp"
#include "ServerSocket.hpp"

class Server
{
public:
	Server()
		: mSocket()
		, mRunning(false)
		, mPlayers()
		, mUpdateAllPlayers(false)
	{
	}

	bool Start(int argc, char** argv)
	{
		mSocket.SetSocketPort((argc >= 2) ? static_cast<en::U16>(std::atoi(argv[1])) : DefaultServerPort);
		if (!mSocket.Start())
		{
			return false;
		}

		mRunning = true;

		return true;
	}

	bool Run()
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

	void Stop()
	{
		mRunning = false;

		mSocket.SetBlocking(true);
		sf::Packet stoppingPacket;
		stoppingPacket << static_cast<en::U8>(ServerPacketID::Stopping);
		SendToAllPlayers(stoppingPacket);

		mSocket.Stop();
	}

	void SendToAllPlayers(sf::Packet& packet)
	{
		const en::U32 size = static_cast<en::U32>(mPlayers.size());
		for (en::U32 i = 0; i < size; ++i)
		{
			mSocket.SendPacket(packet, mPlayers[i].remoteAddress, mPlayers[i].remotePort);
		}
	}

	bool IsRunning() const { return mRunning; }

	void UpdateLogic()
	{
		// Nothing yet
	}

	void Tick()
	{
		const en::U32 size = static_cast<en::U32>(mPlayers.size());
		for (en::U32 i = 0; i < size; ++i)
		{
			if (mPlayers[i].positionChanged || mUpdateAllPlayers)
			{
				sf::Packet positionPacket;
				positionPacket << static_cast<en::U8>(ServerPacketID::PlayerPosition);
				positionPacket << mPlayers[i].clientID;
				positionPacket << mPlayers[i].position.x;
				positionPacket << mPlayers[i].position.y;
				SendToAllPlayers(positionPacket);
				mPlayers[i].positionChanged = false;
			}
		}
		mUpdateAllPlayers = false;
	}

private:
	void HandleIncomingPackets()
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
				sf::Packet pongPacket;
				pongPacket << static_cast<en::U8>(ServerPacketID::Pong);
				mSocket.SendPacket(pongPacket, remoteAddress, remotePort);
			} break;
			case ClientPacketID::Pong:
			{
				const en::U32 clientID = GetClientIDFromIpAddress(remoteAddress, remotePort);
				if (clientID != en::U32_Max)
				{
					// TODO
				}
				else
				{
					ignorePacket = true;
				}
			} break;
			case ClientPacketID::Join:
			{
				const en::U32 clientID = GenerateClientID(remoteAddress, remotePort);
				const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
				const bool slotAvailable = (mPlayers.size() < DefaultMaxPlayers);
				const bool blacklisted = IsBlacklisted(remoteAddress);
				if (slotAvailable && !blacklisted && playerIndex == -1)
				{
					Player newPlayer;
					newPlayer.remoteAddress = remoteAddress;
					newPlayer.remotePort = remotePort;
					newPlayer.clientID = clientID;
					newPlayer.position = { en::Random::get<en::F32>(0.0f, 1024.f - 100.0f), en::Random::get<en::F32>(0.0f, 768.0f - 100.0f) };
					newPlayer.positionChanged = true;

					// Give the "first" data of the game to the player
					sf::Packet acceptPacket;
					acceptPacket << static_cast<en::U8>(ServerPacketID::ClientAccept);
					acceptPacket << clientID;
					mSocket.SendPacket(acceptPacket, remoteAddress, remotePort);
					mUpdateAllPlayers = true;

					// Add the new player
					mPlayers.push_back(newPlayer);

					// Inform everyone a new player joined
					sf::Packet playerJoinedPacket;
					playerJoinedPacket << static_cast<en::U8>(ServerPacketID::ClientJoined);
					playerJoinedPacket << newPlayer.clientID;
					playerJoinedPacket << newPlayer.position.x;
					playerJoinedPacket << newPlayer.position.y;
					SendToAllPlayers(playerJoinedPacket);
				}
				else
				{
					sf::Packet rejectPacket;
					rejectPacket << static_cast<en::U8>(ServerPacketID::ClientReject);
					if (!slotAvailable)
					{
						rejectPacket << static_cast<en::U8>(RejectReason::TooManyPlayers);
					}
					else if (blacklisted)
					{
						rejectPacket << static_cast<en::U8>(RejectReason::Blacklisted);
					}
					else
					{
						rejectPacket << static_cast<en::U8>(RejectReason::UnknownError);
						// Might be :
						// - Already connected
					}
					mSocket.SendPacket(rejectPacket, remoteAddress, remotePort);
				}

			} break;
			case ClientPacketID::Leave:
			{
				sf::Packet leavePacket;
				leavePacket << static_cast<en::U8>(ServerPacketID::ClientLeft);
				const en::U32 clientID = GetClientIDFromIpAddress(remoteAddress, remotePort);
				if (clientID != en::U32_Max)
				{
					leavePacket << clientID;
					SendToAllPlayers(leavePacket);
				}
				else
				{
					ignorePacket = true;
				}
			} break;

			// Game specific code
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

	en::U32 GenerateClientID(const sf::IpAddress& remoteAddress, en::U16 remotePort) const
	{
		return en::Hash::CombineHash(en::Hash::CRC32(remoteAddress.toString().c_str()), static_cast<en::U32>(remotePort));
	}
	
	// Check with a generated client ID from address/port
	bool IsClientIDAssignedToThisPlayer(en::U32 clientID, const sf::IpAddress& remoteAddress, en::U16 remotePort) const
	{
		return clientID == GenerateClientID(remoteAddress, remotePort);
	}

	// Get ID from known player
	en::U32 GetClientIDFromIpAddress(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::I32* playerIndex = nullptr) const
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

	// Get player index from ID
	en::I32 GetPlayerIndexFromClientID(en::U32 clientID) const
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

	bool IsBlacklisted(const sf::IpAddress& remoteAddress) const
	{
		// TODO
		return false;
	}

private:
	ServerSocket mSocket;
	bool mRunning;
	en::Time mStepTime;
	en::Time mTickTime;

	std::vector<Player> mPlayers;
	bool mUpdateAllPlayers;
};
