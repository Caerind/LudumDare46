#pragma once

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
	Server();

	bool Start(int argc, char** argv);
	void Stop();

	bool Run();
	bool IsRunning() const;

	void UpdateLogic(en::Time dt);
	void Tick(en::Time dt);

private:
	void HandleIncomingPackets();

	en::U32 GenerateClientID(const sf::IpAddress& remoteAddress, en::U16 remotePort) const;
	
	// Check with a generated client ID from address/port
	bool IsClientIDAssignedToThisPlayer(en::U32 clientID, const sf::IpAddress& remoteAddress, en::U16 remotePort) const;

	// Get ID from known player
	en::U32 GetClientIDFromIpAddress(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::I32* playerIndex = nullptr) const;

	// Get player index from ID
	en::I32 GetPlayerIndexFromClientID(en::U32 clientID) const;

	bool IsBlacklisted(const sf::IpAddress& remoteAddress) const { return false; } // TODO

private:
	void AddNewSeed(const en::Vector2f& position);

private:
	void SendToAllPlayers(sf::Packet& packet);

	void SendPingPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort);
	void SendPongPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort);
	void SendConnectionAcceptedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::U32 clientID);
	void SendConnectionRejectedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, RejectReason reason);
	void SendClientJoinedPacket(en::U32 clientID, const std::string& nickname, const Chicken& chicken);
	void SendClientLeftPacket(en::U32 clientID);
	void SendServerStopPacket();
	void SendUpdateChickenPacket(en::U32 clientID, const Chicken& chicken);
	void SendAddSeedPacket(const Seed& seed);
	void SendRemoveSeedPacket(en::U32 seedID);

private:
	ServerSocket mSocket;
	bool mRunning;
	en::Time mStepTime;
	en::Time mTickTime;

	std::vector<Player> mPlayers;
	std::vector<Seed> mSeeds;
};
