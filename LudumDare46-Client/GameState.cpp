#include "GameState.hpp"

#include "ConnectingState.hpp"

GameState::GameState(en::StateManager& manager)
	: en::State(manager)
{
	mLocalPosition = { 0.0f, 0.0f };
	mInvalidLocalPosition = true;
}

bool GameState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();
	return false;
}

bool GameState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();

	HandleIncomingPackets();

	en::I32 moved = 0;
	en::Vector2f movement(0.0f, 0.0f);
	const en::F32 speed = 100.0f;
	const en::F32 time = dt.asSeconds();
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
	{
		movement.y -= speed * time;
		moved++;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
	{
		movement.y += speed * time;
		moved++;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
	{
		movement.x -= speed * time;
		moved++;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
	{
		movement.x += speed * time;
		moved++;
	}
	if (moved >= 2)
	{
		movement.normalize();
	}
	if (moved >= 1 && GameSingleton::mClient.IsRunning() && GameSingleton::mClient.IsConnected())
	{
		mLocalPosition += movement;

		sf::Packet positionPacket;
		positionPacket << static_cast<en::U8>(ClientPacketID::PlayerPosition);
		positionPacket << GameSingleton::mClient.GetClientID();
		positionPacket << mLocalPosition.x;
		positionPacket << mLocalPosition.y;
		GameSingleton::mClient.SendPacket(positionPacket);
	}

	return false;
}

void GameState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		target.draw(mPlayers[i].shape);
	}
}

void GameState::HandleIncomingPackets()
{
	if (GameSingleton::mClient.IsRunning())
	{
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
				// TODO
			} break;
			case ServerPacketID::ClientAccept:
			{
				LogWarning(en::LogChannel::All, 6, "ClientAccept received ??%s", "");
			} break;
			case ServerPacketID::ClientReject:
			{
				en::U32 rejectReason;
				packet >> rejectReason;
				GameSingleton::mClient.SetClientID(en::U32_Max);
				GameSingleton::mApplication->PushState<ConnectingState>();
			} break;
			case ServerPacketID::ClientJoined:
			{
				en::U32 clientID;
				packet >> clientID;
				const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
				if (playerIndex == -1)
				{
					Player newPlayer;
					newPlayer.shape.setSize({ 100.0f, 100.0f });
					newPlayer.shape.setFillColor(sf::Color::Green);
					newPlayer.shape.setPosition({ -123456789.0f, -123456789.0f }); // Invalid pos
					newPlayer.clientID = clientID;

					mPlayers.push_back(newPlayer);
				}
				else
				{
					LogWarning(en::LogChannel::All, 6, "Client already joined ??%s", "");
				}
			} break;
			case ServerPacketID::ClientLeft:
			{
				en::U32 clientID;
				packet >> clientID;
				const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
				if (playerIndex >= 0)
				{
					mPlayers.erase(mPlayers.begin() + static_cast<en::U32>(playerIndex));
				}
				else
				{
					LogWarning(en::LogChannel::All, 6, "Client already left or not logged ??%s", "");
				}
			} break;
			case ServerPacketID::Stopping:
			{
				GameSingleton::mClient.SetClientID(en::U32_Max);
				GameSingleton::mApplication->PushState<ConnectingState>();
			} break;
			case ServerPacketID::PlayerPosition:
			{
				en::U32 clientID;
				en::Vector2f position;
				packet >> clientID >> position.x >> position.y;
				const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
				if (playerIndex >= 0)
				{
					mPlayers[playerIndex].shape.setPosition(en::toSF(position));
					if (mInvalidLocalPosition)
					{
						mLocalPosition = position;
						mInvalidLocalPosition = false;
					}
				}
				else
				{
					LogWarning(en::LogChannel::All, 6, "Unknown player moved %d", packetIDRaw);
				}
			} break;
			default:
			{
				LogWarning(en::LogChannel::All, 6, "Unknown ClientPacketID %d received", packetIDRaw);
			} break;
			}
		}
	}
}

en::I32 GameState::GetPlayerIndexFromClientID(en::U32 clientID) const
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
