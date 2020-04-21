#include "MenuState.hpp"

#include "ConnectingState.hpp"
#include "ProfState.hpp"

#include <Enlivengine/Application/ResourceManager.hpp>

MenuState::MenuState(en::StateManager& manager)
	: en::State(manager)
{
	mSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("play_button").Get());
	mSprite.setOrigin(mSprite.getGlobalBounds().width * 0.5f, mSprite.getGlobalBounds().height * 0.5f);
	mSprite.setPosition(512, 650);

	mNicknameInput.setFont(en::ResourceManager::GetInstance().Get<en::Font>("MainFont").Get());
	mNicknameInput.setCharacterSize(24);
	mNicknameInput.setPosition(324.0f, 424.0f);
	mNicknameInput.setFillColor(sf::Color::White);
	mNicknameInput.setOutlineColor(sf::Color::Black);
	mNicknameInput.setOutlineThickness(1.0f);

	if (GameSingleton::mMusic.IsValid())
	{
		if (GameSingleton::mMusic.GetMusicID() != en::Hash::CRC32("menu_soundtrack"))
		{
			GameSingleton::mMusic.Stop();
			GameSingleton::mMusic = en::AudioSystem::GetInstance().PlayMusic("menu_soundtrack");
		}
		else
		{
			// Keep playing
		}
	}
	else
	{
		GameSingleton::mMusic = en::AudioSystem::GetInstance().PlayMusic("menu_soundtrack");
	}
	if (GameSingleton::mMusic.IsValid())
	{
		GameSingleton::mMusic.SetVolume(0.12f);
	}
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

	if (event.type == sf::Event::TextEntered)
	{
		if (event.text.unicode < 128 && GameSingleton::mInputNickname.size() < 12)
		{
			char c = static_cast<char>(event.text.unicode);
			if (c == '\b')
			{
				if (GameSingleton::mInputNickname.size() > 0)
				{
					GameSingleton::mInputNickname.pop_back();
				}
			}
			else if (c == '\t' || c == ' ' || c == '\0')
			{
			}
			else if (c == '\r')
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
			else
			{
				GameSingleton::mInputNickname += c;
			}

			mNicknameInput.setString(GameSingleton::mInputNickname);
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

	target.draw(mNicknameInput);
	

	// Cursor
	GameSingleton::mCursor.setPosition(en::toSF(GameSingleton::mApplication->GetWindow().getCursorPosition()));
	GameSingleton::mCursor.setTextureRect(sf::IntRect(0, 0, 32, 32));
	target.draw(GameSingleton::mCursor);
}
