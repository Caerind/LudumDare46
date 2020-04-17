#pragma once

#include <SFML/Network.hpp>
#include <Common.hpp>

#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/Math/Random.hpp> // Random for ClientID

class ClientSocket
{
public:
	ClientSocket()
		: mSocket()
		, mServerAddress(DefaultServerAddress)
		, mServerPort(DefaultServerPort)
		, mRunning(false)
		, mClientID(en::U32_Max)
	{
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
		LogInfo(en::LogChannel::All, 5, "Started on port %d", mSocket.getLocalPort());
		return true;
	}

	void Stop()
	{
		LogInfo(en::LogChannel::All, 5, "Stopping...%s", "");
		mSocket.unbind();
		mRunning = false;
		LogInfo(en::LogChannel::All, 5, "Stopped%s", "");
	}

	void SetBlocking(bool blocking) { mSocket.setBlocking(blocking); }
	bool IsBlocking() const { mSocket.isBlocking(); }

	void SendPacket(sf::Packet& packet) 
	{ 
		mSocket.send(packet, mServerAddress, mServerPort); 
	}

	bool PollPacket(sf::Packet& packet)
	{
		static sf::IpAddress remoteAddress;
		static en::U16 remotePort;
		while (mSocket.receive(packet, remoteAddress, remotePort) == sf::Socket::Done)
		{
			// Someone that is not the server is sending us packet
			if (remotePort != mServerPort || remoteAddress != mServerAddress)
			{
				// Ignore them for now
				continue;
			}

			return true;
		}
		return false;
	}

	const sf::IpAddress& GetServerAddress() const { return mServerAddress; }
	void SetServerAddress(const sf::IpAddress& serverAddress) { mServerAddress = serverAddress; }
	en::U16 GetServerPort() const { return mServerPort; }
	void SetServerPort(en::U16 serverPort) { mServerPort = serverPort; }

	bool IsRunning() const { return mRunning; }
	bool IsConnected() const { return mClientID != en::U32_Max; }
	en::U32 GetClientID() const { return mClientID; }
	void SetClientID(en::U32 clientID) { mClientID = clientID; }

private:
	sf::UdpSocket mSocket;
	sf::IpAddress mServerAddress;
	en::U16 mServerPort;
	bool mRunning;
	en::U32 mClientID;
};