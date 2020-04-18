#include "GameState.hpp"

#include "ConnectingState.hpp"

GameState::GameState(en::StateManager& manager)
	: en::State(manager)
{
	LogInfo(en::LogChannel::All, 2, "Switching to GameState%s", "");
}

bool GameState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	if (event.type == sf::Event::Closed)
	{
		GameSingleton::SendLeavePacket();
	}

	return false;
}

bool GameState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();
	LogInfo(en::LogChannel::All, 1, "GameState", "");

	GameSingleton::HandleIncomingPackets();
	if (GameSingleton::mClient.IsConnected())
	{
		GameSingleton::mLastPacketTime += dt;
		if (GameSingleton::mLastPacketTime > DefaultTimeout)
		{
			GameSingleton::mClient.SetClientID(en::U32_Max);
			GameSingleton::mClient.Stop();
			clearStates();
		}
	}

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
	if (moved >= 1)
	{
		GameSingleton::mLocalPosition += movement;
		GameSingleton::SendPositionPacket(GameSingleton::mLocalPosition);
	}

	return false;
}

void GameState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();
	const en::U32 size = static_cast<en::U32>(GameSingleton::mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		target.draw(GameSingleton::mPlayers[i].shape);
	}
}
