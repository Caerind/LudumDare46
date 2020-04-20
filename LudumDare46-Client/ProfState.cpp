#include "ProfState.hpp"

#include "ConnectingState.hpp"

ProfState::ProfState(en::StateManager& manager)
	: en::State(manager)
{
	mSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("prof_bucket1").Get());
}

bool ProfState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	return false;
}

bool ProfState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();

	static en::U32 i = 0;
	static en::Time timer = en::Time::Zero;

#ifdef ENLIVE_DEBUG
	static en::Time limit = en::seconds(10.0f);
#else
	static en::Time limit = en::seconds(10.0f);
#endif

	timer += dt;
	if (timer > limit)
	{
		timer = en::Time::Zero;
		i++;
		
		if (i == 0)
		{
			//
		}
		else if (i == 1)
		{
			mSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("prof_bucket2").Get());
		}
		else if (i == 2)
		{
			GameSingleton::mIntroDone = true;

			clearStates();
			pushState<ConnectingState>();
		}
	}

	return false;
}

void ProfState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();

	target.draw(mSprite);
}
