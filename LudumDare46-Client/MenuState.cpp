#include "MenuState.hpp"

#include "ConnectingState.hpp"
#include "ProfState.hpp"

MenuState::MenuState(en::StateManager& manager)
	: en::State(manager)
{
}

bool MenuState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	return false;
}

bool MenuState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();

	static en::Time timer = en::Time::Zero;
	timer += dt;
	if (timer > en::seconds(4.0f))
	{
		timer = en::Time::Zero;

		clearStates();
		if (GameSingleton::mIntroDone)
		{
			pushState<ConnectingState>();
		}
		else
		{
			pushState<ProfState>();
		}
	}

	return false;
}

void MenuState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();

	en::View previousView = target.getView();

	static const en::Vector2f mapSize = en::Vector2f(DefaultMapSizeX, DefaultMapSizeY);
	en::View view;
	view.setCenter(mapSize);
	view.setCenter(mapSize * 0.5f);
	target.setView(view.getHandle());
	GameSingleton::mMap.render(target);

	target.setView(previousView.getHandle());

	static bool init = false;
	static sf::Sprite screen;
	if (!init)
	{
		screen.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("screen_home").Get());
		init = true;
	}
	target.draw(screen);
}
