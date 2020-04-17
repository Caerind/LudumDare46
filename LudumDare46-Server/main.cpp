#include <Enlivengine/System/Log.hpp>


#include <cstdlib> // Test
#include <SFML/Network.hpp> // Test
#include <Common.hpp> // Test
#include <Enlivengine/Math/Vector2.hpp>
#include <vector>

struct Player
{
	sf::IpAddress remoteAddress;
	en::U16 remotePort;
	en::U32 clientID;

	en::Vector2f position;
};

class Server
{
public:
	Server()
		: mSocket()
		, mSocketPort(DefaultServerPort)
		, mRunning(false)
	{
	}

	bool Run()
	{
		if (!Start())
		{
			return false;
		}

		while (IsRunning())
		{
			//LogInfo(en::LogChannel::All, 2, "Ticks%s", "");

			sf::Packet receivedPacket;
			sf::IpAddress remoteAddress;
			en::U16 remotePort;
			while (mSocket.receive(receivedPacket, remoteAddress, remotePort) == sf::Socket::Done)
			{
				en::U32 packetIDRaw;
				receivedPacket >> packetIDRaw;

				LogInfo(en::LogChannel::All, 2, "Received PacketID (%d) from %s:%d", packetIDRaw, remoteAddress.toString().c_str(), remotePort);

				const PacketID packetID = static_cast<PacketID>(packetIDRaw);
				switch (packetID)
				{
				case PacketID::Position:
				{
					en::U32 clientID;
					en::Vector2f position;
					receivedPacket >> clientID >> position.x >> position.y;

					bool playerFound = false;
					const en::U32 size = static_cast<en::U32>(mPlayers.size());
					for (en::U32 i = 0; i < size && !playerFound; ++i)
					{
						if (mPlayers[i].clientID == clientID)
						{
							// TODO : Detect cheater ?
							//assert(mPlayers[i].remoteAddress == remoteAddress);
							//assert(mPlayers[i].remotePort == remotePort);
							mPlayers[i].position = position;
							playerFound = true;
						}
					}
					if (!playerFound)
					{
						Player newPlayer;
						newPlayer.remoteAddress = remoteAddress;
						newPlayer.remotePort = remotePort;
						newPlayer.clientID = clientID;
						newPlayer.position = position;
						mPlayers.push_back(newPlayer);
					}
				} break;

				default:
					LogWarning(en::LogChannel::All, 8, "Invalid PacketID (%d) received from %s:%d", packetID, remoteAddress.toString().c_str(), remotePort);
					break;
				}
			}

			const en::U32 size = static_cast<en::U32>(mPlayers.size());
			for (en::U32 i = 0; i < size; ++i)
			{
				sf::Packet positionPacket;
				positionPacket << static_cast<en::U32>(PacketID::UpdatePosition);
				positionPacket << mPlayers[i].clientID;
				positionPacket << mPlayers[i].position.x;
				positionPacket << mPlayers[i].position.y;
				for (en::U32 j = 0; j < size; ++j)
				{
					sf::Packet copyPacket = positionPacket;
					mSocket.send(copyPacket, mPlayers[j].remoteAddress, mPlayers[j].remotePort);
				}
			}

			sf::sleep(sf::milliseconds(20));
		}

		return true;
	}

	unsigned short GetSocketPort() const { return mSocketPort; }
	void SetSocketPort(unsigned short socketPort) { mSocketPort = socketPort; }

	bool IsRunning() const { return mRunning; }

private:
	bool Start()
	{
		LogInfo(en::LogChannel::All, 5, "Starting%s", "");
		mSocket.setBlocking(true);
		if (mSocket.bind(mSocketPort) != sf::Socket::Done)
		{
			LogError(en::LogChannel::All, 5, "Can't bind port %d", mSocketPort);
			return false;
		}
		mSocket.setBlocking(false);
		mRunning = true;
		LogInfo(en::LogChannel::All, 5, "Started at %s:%d", sf::IpAddress::getPublicAddress().toString().c_str(), mSocket.getLocalPort());
		return true;
	}

	void Stop()
	{
		LogInfo(en::LogChannel::All, 5, "Stopping%s", "");
		mRunning = false;
		LogInfo(en::LogChannel::All, 5, "Stopped%s", "");
	}

private:
	sf::UdpSocket mSocket;
	unsigned short mSocketPort;
	bool mRunning;

	std::vector<Player> mPlayers;
};

int main(int argc, char** argv)
{
	en::LogManager::GetInstance().Initialize();

	Server server;
	server.SetSocketPort((argc >= 2) ? static_cast<unsigned short>(std::atoi(argv[1])) : DefaultServerPort);
	server.Run();
	return 0;
}
