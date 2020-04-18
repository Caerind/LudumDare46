#include "GameState.hpp"

#include "ConnectingState.hpp"

GameState::GameState(en::StateManager& manager)
	: en::State(manager)
{
	LogInfo(en::LogChannel::All, 2, "Switching to GameState%s", "");

	mView.setSize(1024.0f, 768.0f);
	mView.setCenter(1024.0f * 0.5f, 768.0f * 0.5f);
}

bool GameState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	if (event.type == sf::Event::Closed)
	{
		GameSingleton::SendLeavePacket();
	}

	if (event.type == sf::Event::MouseButtonPressed)
	{
		const en::Vector2f cursorPos = GameSingleton::mApplication->GetWindow().getCursorPositionView(mView);
		GameSingleton::SendDropSeedPacket(cursorPos);
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

	const en::U32 playerSize = static_cast<en::U32>(GameSingleton::mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		if (GameSingleton::mPlayers[i].clientID == GameSingleton::mClient.GetClientID())
		{
			mView.setCenter(GameSingleton::mPlayers[i].chicken.position);
		}
	}

	return false;
}

void GameState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();

	const sf::View previousView = target.getView();
	target.setView(mView.getHandle());

	static bool seedInitialized = false;
	static sf::CircleShape seed;
	if (!seedInitialized)
	{
		seed.setRadius(3.0f);
		seed.setFillColor(sf::Color::Green);
		seed.setOrigin(3.0f, 3.0f);
		seedInitialized = true;
	}

	static bool playerInitialized = false;
	static sf::CircleShape player;
	if (!playerInitialized)
	{
		player.setRadius(30.0f);
		player.setFillColor(sf::Color::Red);
		player.setOrigin(30.0f, 30.0f);
		playerInitialized = true;
	}

	const en::U32 seedSize = static_cast<en::U32>(GameSingleton::mSeeds.size());
	for (en::U32 i = 0; i < seedSize; ++i)
	{
		seed.setPosition(en::toSF(GameSingleton::mSeeds[i].position));
		target.draw(seed);
	}

	const en::U32 playerSize = static_cast<en::U32>(GameSingleton::mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		if (GameSingleton::mPlayers[i].clientID == GameSingleton::mClient.GetClientID())
		{
			player.setFillColor(sf::Color::Yellow);
		}
		player.setPosition(en::toSF(GameSingleton::mPlayers[i].chicken.position));
		target.draw(player);
	}

	target.setView(previousView);
}
