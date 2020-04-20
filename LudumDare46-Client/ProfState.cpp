#include "ProfState.hpp"

#include <Enlivengine/System/Macros.hpp>

#include "ConnectingState.hpp"

static constexpr const char* texts[] = {
	"Hello there!",
	"Welcome to the real world!",
	"My name is Bucket!",
	"People call me the chicken Prof!",
	"This world is inhabited by creatures called chickens!",
	"For some people, chickens are pets.",
	"Others use them for fights.",
	"Myself...",
	"...",
	"I frie chicken as a profession.",
	"...",
	"I will give you your first chicken.",
	"You have to be the very best!",
	"Fight against the others and KEEP IT ALIVE!"
};
static constexpr en::U32 textCount = ENLIVE_ARRAY_SIZE(texts);

static sf::Time limitClick = sf::seconds(0.3f);
static sf::Time limit = sf::seconds(2.5f);

ProfState::ProfState(en::StateManager& manager)
	: en::State(manager)
{
	mSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("intro_bucket").Get());
	mStates = 0;
	mText.setFont(en::ResourceManager::GetInstance().Get<en::Font>("MainFont").Get());
	mText.setCharacterSize(40);
	mText.setPosition(50.0f, 650.0f);
	mText.setFillColor(sf::Color::White);
	mText.setOutlineColor(sf::Color::Black);
	mText.setOutlineThickness(4.0f);
	mText.setString(texts[mStates]);
}

bool ProfState::handleEvent(const sf::Event& event)
{
	if (event.type == sf::Event::MouseButtonPressed && mClock.getElapsedTime() >= limitClick)
	{
		mClock.restart();
		NextState();
	}
	return false;
}

bool ProfState::update(en::Time dt)
{
	if (mClock.getElapsedTime() >= limit)
	{
		mClock.restart();
		NextState();
	}
	return false;
}

void ProfState::render(sf::RenderTarget& target)
{
	target.draw(mSprite);
	target.draw(mText);
}

void ProfState::NextState()
{
	mStates++;
	if (mStates >= textCount)
	{
		GameSingleton::mIntroDone = true;

		clearStates();
		pushState<ConnectingState>();
	}
	else
	{
		mText.setString(texts[mStates]);

		en::SoundPtr sound = en::AudioSystem::GetInstance().PlaySound("type");
		if (sound.IsValid())
		{
			sound.SetVolume(0.25f);
		}
	}
}
