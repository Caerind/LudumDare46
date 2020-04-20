#include "IntroState.hpp"

IntroState::IntroState(en::StateManager& manager)
	: en::State(manager)
{
}

bool IntroState::handleEvent(const sf::Event& event)
{
	return false;
}

bool IntroState::update(en::Time dt)
{
	return false;
}

void IntroState::render(sf::RenderTarget& target)
{
}
