#pragma once

#include <Enlivengine/Application/Application.hpp>
#include <Enlivengine/Application/StateManager.hpp>

#include <fstream>

#include "GameSingleton.hpp"
#include "GameState.hpp"

class ConnectingState : public en::State
{
public:
	ConnectingState(en::StateManager& manager);
	
	bool handleEvent(const sf::Event& event);

	bool update(en::Time dt);

	void render(sf::RenderTarget& target);

private:
	en::Time mWaitingTime;
	std::string mTextString;
};