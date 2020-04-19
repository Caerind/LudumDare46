#include "GameState.hpp"

#include "ConnectingState.hpp"

GameState::GameState(en::StateManager& manager)
	: en::State(manager)
{
	GameSingleton::mView.setSize(1024.0f, 768.0f);
	GameSingleton::mView.setCenter(1024.0f * 0.5f, 768.0f * 0.5f);

	GameSingleton::mMap.load();
}

bool GameState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	if (event.type == sf::Event::MouseButtonPressed)
	{
		const en::Vector2f cursorPos = GameSingleton::mApplication->GetWindow().getCursorPositionView(GameSingleton::mView);
		GameSingleton::SendDropSeedPacket(cursorPos);
	}

	return false;
}

bool GameState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();

	// Network
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

	// Bullets
	en::U32 bulletSize = static_cast<en::U32>(GameSingleton::mBullets.size());
	for (en::U32 i = 0; i < bulletSize; )
	{
		const en::F32 distance = dt.asSeconds() * DefaultProjectileSpeed;
		GameSingleton::mBullets[i].position += en::Vector2f::polar(GameSingleton::mBullets[i].rotation, distance);
		GameSingleton::mBullets[i].remainingDistance -= distance;
		if (GameSingleton::mBullets[i].remainingDistance <= 0.0f)
		{
			GameSingleton::mBullets.erase(GameSingleton::mBullets.begin() + i);
			bulletSize--;
		}
		else
		{
			i++;
		}
	}

	// Center view on current player
	{
		const en::U32 playerSize = static_cast<en::U32>(GameSingleton::mPlayers.size());
		for (en::U32 i = 0; i < playerSize; ++i)
		{
			if (GameSingleton::mPlayers[i].clientID == GameSingleton::mClient.GetClientID())
			{
				GameSingleton::mView.setCenter(GameSingleton::mPlayers[i].chicken.position);
			}
		}
	}

	return false;
}

void GameState::render(sf::RenderTarget& target)
{
	ENLIVE_PROFILE_FUNCTION();

	const sf::View previousView = target.getView();
	target.setView(GameSingleton::mView.getHandle());

	// Maps
	GameSingleton::mMap.render(target);

	// Items
	static bool itemInitialized = false;
	static sf::Sprite itemSprite;
	if (!itemInitialized)
	{
		itemSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("loots").Get());
		itemSprite.setTextureRect(sf::IntRect(0, 0, 32, 32));
		itemSprite.setOrigin(16.0f, 16.0f);
		itemSprite.setScale(1.5f, 1.5f);
		itemInitialized = true;
	}
	const en::U32 itemSize = static_cast<en::U32>(GameSingleton::mItems.size());
	for (en::U32 i = 0; i < itemSize; ++i)
	{
		bool valid = true;
		switch (GameSingleton::mItems[i].item)
		{
		case ItemID::Shuriken: itemSprite.setTextureRect(sf::IntRect(0, 0, 32, 32)); break;
		case ItemID::Laser: itemSprite.setTextureRect(sf::IntRect(32, 0, 32, 32)); break;
		default: valid = false; break;
		}
		if (valid)
		{
			itemSprite.setPosition(en::toSF(GameSingleton::mItems[i].position));
			target.draw(itemSprite);
		}
	}

	// Seeds
	static bool seedInitialized = false;
	static sf::Sprite seedSprite;
	if (!seedInitialized)
	{
		seedSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("seeds").Get());
		seedSprite.setOrigin(8.0f, 8.0f);
		seedSprite.setScale(3.0f, 3.0f);
		seedInitialized = true;
	}
	const en::U32 seedSize = static_cast<en::U32>(GameSingleton::mSeeds.size());
	for (en::U32 i = 0; i < seedSize; ++i)
	{
		seedSprite.setPosition(en::toSF(GameSingleton::mSeeds[i].position));
		target.draw(seedSprite);
	}

	// Bullets
	static bool bulletInitialized = false;
	static sf::Sprite bulletSprite;
	if (!bulletInitialized)
	{
		bulletSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("bullets").Get());
		bulletSprite.setTextureRect(sf::IntRect(0, 0, 16, 16));
		bulletSprite.setOrigin(8.0f, 8.0f);
		bulletInitialized = true;
	}
	const en::U32 bulletSize = static_cast<en::U32>(GameSingleton::mBullets.size());
	for (en::U32 i = 0; i < bulletSize; ++i)
	{
		bool valid = true;
		switch (GameSingleton::mBullets[i].itemID)
		{
		case ItemID::Shuriken: bulletSprite.setTextureRect(sf::IntRect(0, 0, 16, 16)); break;
		case ItemID::Laser: bulletSprite.setTextureRect(sf::IntRect(16, 0, 16, 16)); break;
		default: valid = false; break;
		}
		if (valid)
		{
			bulletSprite.setPosition(en::toSF(GameSingleton::mBullets[i].position));
			bulletSprite.setRotation(GameSingleton::mBullets[i].rotation + 90.0f);
			target.draw(bulletSprite);
		}
	}

	// Players
	static bool chickenBodyInitialized = false;
	static sf::Sprite chickenBodySprite;
	if (!chickenBodyInitialized)
	{
		chickenBodySprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("chicken_body").Get());
		chickenBodySprite.setOrigin(32.0f, 32.0f);
		chickenBodyInitialized = true;
	}
	const en::U32 playerSize = static_cast<en::U32>(GameSingleton::mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		en::Color color = en::Color::White;
		if (GameSingleton::mPlayers[i].chicken.lifeMax >= 0.0f)
		{
			const en::F32 redFactor = 1.0f - (GameSingleton::mPlayers[i].chicken.life / GameSingleton::mPlayers[i].chicken.lifeMax);
			color.r = static_cast<en::U8>(en::Math::Clamp(255.0f * redFactor, 0.0f, 255.0f));
		}
		chickenBodySprite.setColor(en::toSF(color));
		chickenBodySprite.setPosition(en::toSF(GameSingleton::mPlayers[i].chicken.position));
		target.draw(chickenBodySprite);
		target.draw(GameSingleton::mPlayers[i].sprite);
	}

	// Cursor
	GameSingleton::mCursor.setPosition(en::toSF(GameSingleton::mApplication->GetWindow().getCursorPositionView(GameSingleton::mView)));
	GameSingleton::mCursor.setTextureRect(sf::IntRect(32, 0, 32, 32));
	target.draw(GameSingleton::mCursor);

	target.setView(previousView);
}
