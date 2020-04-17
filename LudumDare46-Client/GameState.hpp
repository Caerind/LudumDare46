#pragma once

#include <Enlivengine/Application/StateManager.hpp>
#include <Enlivengine/Math/Vector2.hpp>

#include <Common.hpp>

#include "Player.hpp"

class GameState : public en::State
{
public:
	GameState(en::StateManager& manager);

	bool handleEvent(const sf::Event& event);

	bool update(en::Time dt);

	void render(sf::RenderTarget& target);

private:
	void HandleIncomingPackets();


	en::I32 GetPlayerIndexFromClientID(en::U32 clientID) const;

private:
	std::vector<Player> mPlayers;
	en::Vector2f mLocalPosition;
	bool mInvalidLocalPosition;
};