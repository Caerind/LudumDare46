#pragma once

#include <Enlivengine/Application/StateManager.hpp>
#include <Enlivengine/Math/Vector2.hpp>
#include <Enlivengine/Graphics/View.hpp>
#include <Enlivengine/Application/AudioSystem.hpp>

#include <SFML/Graphics/Text.hpp>

#include <Common.hpp>

#include "Player.hpp"

class IntroState : public en::State
{
public:
	IntroState(en::StateManager& manager);

	bool handleEvent(const sf::Event& event);

	bool update(en::Time dt);

	void render(sf::RenderTarget& target);
};