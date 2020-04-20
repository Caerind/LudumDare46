#include "MenuState.hpp"

#include "ConnectingState.hpp"
#include "ProfState.hpp"

#include <Enlivengine/Application/ResourceManager.hpp>

MenuState::MenuState(en::StateManager& manager)
	: en::State(manager)
{
	mSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("play_button").Get());
	mSprite.setOrigin(mSprite.getGlobalBounds().width * 0.5f, mSprite.getGlobalBounds().height * 0.5f);
	mSprite.setPosition(512, 550);
}

bool MenuState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	if (event.type == sf::Event::MouseButtonPressed)
	{
		if (mSprite.getGlobalBounds().contains(en::toSF(GameSingleton::mApplication->GetWindow().getCursorPosition())))
		{
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
	}

	return false;
}

bool MenuState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();

	return false;
}

void MenuState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();

	en::View previousView = target.getView();

	static const en::Vector2f mapSize = en::Vector2f(DefaultMapSizeX, DefaultMapSizeY);
	en::View view;
	view.setSize(mapSize);
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

	target.draw(mSprite);
}
