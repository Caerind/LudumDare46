#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/Application/Application.hpp>
#include <Enlivengine/Application/Window.hpp>
#include <Enlivengine/Graphics/View.hpp>

#include <Enlivengine/Application/StateManager.hpp> // TestState
#include <Enlivengine/Math/Random.hpp> // TestState
#include <SFML/Graphics/RectangleShape.hpp> // TestState
#include <SFML/Network.hpp> // Client
#include <Common.hpp> // Test

class Client
{
public:
	Client()
		: mSocket()
		, mServerAddress()
		, mServerPort(0)
		, mRunning(false)
		, mClientID(en::Random::get<en::U32>(0, 1000000))
		, mDelayClock()
	{
		LogInfo(en::LogChannel::All, 5, "ClientID: %d", mClientID);
	}

	bool Start()
	{
		LogInfo(en::LogChannel::All, 5, "Starting...%s", "");
		mSocket.setBlocking(true);
		if (mSocket.bind(sf::Socket::AnyPort) != sf::Socket::Done)
		{
			LogError(en::LogChannel::All, 5, "Can't find an available port%s", "");
			return false;
		}
		mSocket.setBlocking(false);
		mRunning = true;
		LogInfo(en::LogChannel::All, 5, "Bind on port %d", mSocket.getLocalPort());
		LogInfo(en::LogChannel::All, 5, "Started%s", "");
		return true;
	}

	sf::UdpSocket& GetSocket() { return mSocket; }

	void SendPacket(sf::Packet& packet) 
	{ 
		mSocket.send(packet, mServerAddress, mServerPort); 
	}

	bool PollPacket(sf::Packet& packet)
	{
		static sf::IpAddress remoteAddress;
		static en::U16 remotePort;
		if (mSocket.receive(packet, remoteAddress, remotePort) == sf::Socket::Done)
		{
			// Crash if something is "weird"...
			assert(remoteAddress == mServerAddress);
			assert(remotePort == mServerPort); 
			return true;
		}
		else
		{
			return false;
		}
	}

	void Stop()
	{
		LogInfo(en::LogChannel::All, 5, "Stopping...%s", "");
		mRunning = false;
		LogInfo(en::LogChannel::All, 5, "Stopped%s", "");
	}

	const sf::IpAddress& GetServerAddress() const { return mServerAddress; }
	void SetServerAddress(const sf::IpAddress& serverAddress) { mServerAddress = serverAddress; }
	en::U16 GetServerPort() const { return mServerPort; }
	void SetServerPort(en::U16 serverPort) { mServerPort = serverPort; }

	bool IsRunning() const { return mRunning; }
	en::U32 GetClientID() const { return mClientID; }

private:
	sf::UdpSocket mSocket;
	sf::IpAddress mServerAddress;
	en::U16 mServerPort;
	bool mRunning;
	en::U32 mClientID;
	en::Clock mDelayClock;
};

struct Player
{
	sf::RectangleShape shape;
	en::U32 clientID;
};

class TestState : public en::State
{
public:
	TestState(en::StateManager& manager)
		: en::State(manager)
		, mClient()
		, mPlayerPosition()
	{
		mClient.SetServerAddress(sf::IpAddress(DefaultServerAddress));
		mClient.SetServerPort(DefaultServerPort);
		const bool result = mClient.Start();
		assert(result);

		mPlayerPosition = en::Vector2f(
			en::Random::get<en::F32>(0.0f, 1024.0f - 100.0f),
			en::Random::get<en::F32>(0.0f, 768.0f - 100.0f)
		);
	}
	
	bool handleEvent(const sf::Event& event)
	{
		ENLIVE_PROFILE_FUNCTION();

		return false;
	}

	bool update(en::Time dt)
	{
		ENLIVE_PROFILE_FUNCTION();
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
		{
			mPlayerPosition.x -= dt.asSeconds() * 50.0f;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			mPlayerPosition.x += dt.asSeconds() * 50.0f;
		}

		/*
		if (mClient.IsRunning())
		{
			sf::Packet positionPacket;
			positionPacket << static_cast<en::U32>(PacketID::Position);
			positionPacket << mClient.GetClientID();
			positionPacket << mPlayerPosition.x;
			positionPacket << mPlayerPosition.y;
			mClient.SendPacket(positionPacket);

			sf::Packet receivedPacket;
			while (mClient.PollPacket(receivedPacket))
			{
				en::U32 packetIDRaw;
				receivedPacket >> packetIDRaw;

				const PacketID packetID = static_cast<PacketID>(packetIDRaw);
				switch (packetID)
				{
				case PacketID::UpdatePosition:
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
							mPlayers[i].shape.setPosition(en::toSF(position));
							playerFound = true;
						}
					}
					if (!playerFound)
					{
						Player newPlayer;
						newPlayer.shape.setSize({ 100.0f, 100.0f });
						newPlayer.shape.setFillColor(sf::Color::Green);
						newPlayer.shape.setPosition(en::toSF(position));
						newPlayer.clientID = clientID;
						mPlayers.push_back(newPlayer);
					}

				} break;

				default: LogError(en::LogChannel::All, 10, "Invalid MessageID : %d", messageID); assert(false); break;
				}
			}
		}
		*/

		return false;
	}

	void render(sf::RenderTarget& target)
	{
		ENLIVE_PROFILE_FUNCTION();

		const en::U32 size = static_cast<en::U32>(mPlayers.size());
		for (en::U32 i = 0; i < size; ++i)
		{
			target.draw(mPlayers[i].shape);
		}
	}

private:
	Client mClient;
	en::Vector2f mPlayerPosition;
	std::vector<Player> mPlayers;
};

int main()
{
	en::Application::GetInstance().Initialize();

	en::Application& app = en::Application::GetInstance();
	app.GetWindow().create(sf::VideoMode(1024, 768), "LudumDare46", sf::Style::Titlebar | sf::Style::Close);
	app.GetWindow().getMainView().setCenter({ 512.0f, 384.0f });
	app.GetWindow().getMainView().setSize({ 1024.0f, 768.0f });
	en::PathManager::GetInstance().SetScreenshotPath("Screenshots/"); // TODO

	app.Start<TestState>();

	return 0;
}