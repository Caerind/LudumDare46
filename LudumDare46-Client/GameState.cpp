#include "GameState.hpp"

#include "ConnectingState.hpp"

GameState::GameState(en::StateManager& manager)
	: en::State(manager)
{
	// TODO : Move out
	GameSingleton::mView.setSize(1024.0f, 768.0f);
	GameSingleton::mView.setCenter(1024.0f * 0.5f, 768.0f * 0.5f);
	GameSingleton::mView.setZoom(0.5f);
	GameSingleton::mPlayingState = GameSingleton::PlayingState::Playing;
	
	mPlayerPos = { 0.0f, 0.0f };
}

bool GameState::handleEvent(const sf::Event& event)
{
	ENLIVE_PROFILE_FUNCTION();

	if (GameSingleton::IsPlaying())
	{
		if (event.type == sf::Event::MouseButtonPressed)
		{
			GameSingleton::SendDropSeedPacket(GameSingleton::mApplication->GetWindow().getCursorPositionView(GameSingleton::mView));
		}
	}
	else
	{
		// TODO : Buttons : Respawn & Leave
	}

	return false;
}

bool GameState::update(en::Time dt)
{
	ENLIVE_PROFILE_FUNCTION();
	const en::F32 dtSeconds = dt.asSeconds();

	static en::Time antiTimeout;
	antiTimeout += dt;
	if (antiTimeout >= en::seconds(1.0f))
	{
		GameSingleton::SendPingPacket();
		antiTimeout = en::Time::Zero;
	}

	// Network
	GameSingleton::HandleIncomingPackets();
	if (GameSingleton::HasTimeout(dt) || GameSingleton::IsConnecting())
	{
		clearStates();
	}

	// Bullets
	mShurikenRotation = en::Math::AngleMagnitude(mShurikenRotation + dtSeconds * DefaultShurikenRotDegSpeed);
	en::U32 bulletSize = static_cast<en::U32>(GameSingleton::mBullets.size());
	for (en::U32 i = 0; i < bulletSize; )
	{
		bool remove = GameSingleton::mBullets[i].Update(dtSeconds);
		en::U32 playerHitIndex = en::U32_Max;

		if (!remove)
		{
			const en::U32 size = static_cast<en::U32>(GameSingleton::mPlayers.size());
			for (en::U32 j = 0; j < size && !remove; ++j)
			{
				if (GameSingleton::mBullets[i].clientID != GameSingleton::mPlayers[j].clientID && (GameSingleton::mPlayers[j].chicken.position - GameSingleton::mBullets[i].position).getSquaredLength() < DefaultDetectionRadiusSqr)
				{
					remove = true;
					playerHitIndex = j;
				}
			}
		}

		// Hit some player
		if (playerHitIndex != en::U32_Max) 
		{
			// Add blood
			Blood blood;
			blood.bloodUID = en::Random::get<en::U32>(0, 78); // Don't care, not shared
			blood.position = GameSingleton::mPlayers[playerHitIndex].chicken.position;
			blood.position.x += en::Random::get<en::F32>(-10.0f, +10.0f);
			blood.position.y += en::Random::get<en::F32>(-10.0f, +10.0f);
			blood.remainingTime = en::seconds(en::Random::get<en::F32>(2.0f, 4.0f));
			GameSingleton::mBloods.push_back(blood);

			// Play fire sound
			if (GameSingleton::IsInView(GameSingleton::mPlayers[playerHitIndex].chicken.position))
			{
				// Damage
				en::SoundPtr soundDamage = en::AudioSystem::GetInstance().PlaySound("chicken_damage");
				if (soundDamage.IsValid())
				{
					soundDamage.SetVolume(0.25f);
				}

				// Hit
				en::SoundPtr sound = en::AudioSystem::GetInstance().PlaySound(GetItemSoundHitName(GameSingleton::mBullets[i].itemID));
				if (sound.IsValid())
				{
					sound.SetVolume(0.25f);
				}
			}

			// Compute this here, it will be overwritten by the server
			GameSingleton::mPlayers[playerHitIndex].chicken.life -= DefaultChickenAttack + 1.0f;

			if (GameSingleton::mPlayers[playerHitIndex].chicken.life <= 0.0f && GameSingleton::IsInView(GameSingleton::mPlayers[playerHitIndex].chicken.position))
			{
				// Kill
				en::SoundPtr soundKill = en::AudioSystem::GetInstance().PlaySound("chicken_kill");
				if (soundKill.IsValid())
				{
					soundKill.SetVolume(0.25f);
				}

				if (!GameSingleton::IsClient(GameSingleton::mPlayers[playerHitIndex].clientID))
				{
					// Reward
					en::SoundPtr soundKill = en::AudioSystem::GetInstance().PlaySound("reward");
					if (soundKill.IsValid())
					{
						soundKill.SetVolume(0.25f);
					}
				}
				else
				{
					// Handle main player dying in server switch
				}
			}
		}

		if (remove)
		{
			GameSingleton::mBullets.erase(GameSingleton::mBullets.begin() + i);
			bulletSize--;
		}
		else
		{
			i++;
		}
	}

	// Bloods
	en::U32 bloodSize = static_cast<en::U32>(GameSingleton::mBloods.size());
	for (en::U32 i = 0; i < bloodSize; )
	{
		GameSingleton::mBloods[i].remainingTime -= dt;
		if (GameSingleton::mBloods[i].remainingTime < en::Time::Zero)
		{
			GameSingleton::mBloods.erase(GameSingleton::mBloods.begin() + i);
			bloodSize--;
		}
		else
		{
			i++;
		}
	}

	// Current Player
	{
		static en::Vector2f lastPositionSoundStart;
		static const en::Time moveSoundDuration = en::seconds(0.2f);
		static en::Time lastTimeSinceSoundStart = en::Time::Zero;

		lastTimeSinceSoundStart += dt;

		const en::U32 playerSize = static_cast<en::U32>(GameSingleton::mPlayers.size());
		for (en::U32 i = 0; i < playerSize; ++i)
		{
			if (GameSingleton::IsClient(GameSingleton::mPlayers[i].clientID))
			{
				mPlayerPos = GameSingleton::mPlayers[i].chicken.position;

				// Cam stylé effect
				const en::Vector2f mPos = getApplication().GetWindow().getCursorPositionView(GameSingleton::mView);
				const en::Vector2f pPos = GameSingleton::mPlayers[i].chicken.position;
				en::Vector2f delta = mPos - pPos;
				if (delta.getSquaredLength() > DefaultCameraMaxDistanceSqr)
				{
					delta.setLength(DefaultCameraMaxDistance);
				}
				const en::Vector2f lPos = en::Vector2f::lerp(pPos + delta, pPos, DefaultCameraLerpFactor);
				GameSingleton::mView.setCenter(lPos);

				if (lastTimeSinceSoundStart >= moveSoundDuration && lastPositionSoundStart != pPos)
				{
					lastTimeSinceSoundStart = en::Time::Zero;
					en::AudioSystem::GetInstance().PlaySound("chicken_move").SetVolume(0.05f);
					lastPositionSoundStart = pPos;
				}
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

	// Background
	static bool backgroundInitialized = false;
	static sf::Sprite background;
	if (!backgroundInitialized)
	{
		en::Texture& texture = en::ResourceManager::GetInstance().Get<en::Texture>("water").Get();
		texture.setRepeated(true);
		background.setTexture(texture);
		background.setTextureRect(sf::IntRect(0, 0, 1024, 768));
		backgroundInitialized = true;
	}
	en::I32 x = static_cast<en::I32>(mPlayerPos.x) / 64;
	en::I32 y = static_cast<en::I32>(mPlayerPos.y) / 64;
	background.setPosition(((x - 1) * 64.0f) - 512.0f, ((y - 1) * 64.0f) - 384.0f);
	target.draw(background);

	// Maps
	GameSingleton::mMap.render(target);

	// Bloods
	static bool bloodInitialized = false;
	static sf::Sprite bloodSprite;
	if (!bloodInitialized)
	{
		bloodSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("blood").Get());
		bloodSprite.setTextureRect(sf::IntRect(0, 0, 16, 16));
		bloodSprite.setOrigin(8.0f, 8.0f);
		bloodSprite.setScale(1.5f, 1.5f);
		bloodInitialized = true;
	}
	const en::U32 bloodSize = static_cast<en::U32>(GameSingleton::mBloods.size());
	for (en::U32 i = 0; i < bloodSize; ++i)
	{
		const en::U32 bloodIndex = (GameSingleton::mBloods[i].bloodUID % DefaultBloodCount);
		bloodSprite.setTextureRect(sf::IntRect(bloodIndex * 16, 0, 16, 16));
		bloodSprite.setPosition(en::toSF(GameSingleton::mBloods[i].position));
		target.draw(bloodSprite);
	}

	// Items
	static bool itemInitialized = false;
	static sf::Sprite itemSprite;
	if (!itemInitialized)
	{
		itemSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("loots").Get());
		itemSprite.setOrigin(16.0f, 16.0f);
		itemSprite.setScale(1.5f, 1.5f);
		itemInitialized = true;
	}
	const en::U32 itemSize = static_cast<en::U32>(GameSingleton::mItems.size());
	for (en::U32 i = 0; i < itemSize; ++i)
	{
		if (IsValidItemForAttack(GameSingleton::mItems[i].itemID))
		{
			itemSprite.setTextureRect(GetItemLootTextureRect(GameSingleton::mItems[i].itemID));
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
		if (GameSingleton::IsClient(GameSingleton::mSeeds[i].clientID))
		{
			seedSprite.setPosition(en::toSF(GameSingleton::mSeeds[i].position));
			target.draw(seedSprite);
		}
	}

	// Radar
	static bool radarInitialized = false;
	static sf::Sprite radarSprite;
	if (!radarInitialized)
	{
		radarSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("radar").Get());
		radarSprite.setOrigin(8.0f, 8.0f);
		radarSprite.setScale(1.4f, 1.2f);
		radarInitialized = true;
	}
	en::I32 playerIndex = GameSingleton::GetPlayerIndexFromClientID(GameSingleton::mClient.GetClientID());
	if (playerIndex >= 0)
	{
		const en::Vector2f position = GameSingleton::mPlayers[playerIndex].chicken.position;
		en::F32 bestDistanceSqr = 9999999999.9f;
		en::Vector2f bestPos;
		const en::U32 playerSize = static_cast<en::U32>(GameSingleton::mPlayers.size());
		for (en::U32 i = 0; i < playerSize; ++i)
		{
			if (i != static_cast<en::U32>(playerIndex))
			{
				const en::Vector2f& otherPos = GameSingleton::mPlayers[i].chicken.position;
				const en::Vector2f delta = (otherPos - position);
				const en::F32 distanceSqr = delta.getSquaredLength();
				if (distanceSqr < bestDistanceSqr)
				{
					bestDistanceSqr = distanceSqr;
					bestPos = otherPos;
				}
			}
		}
		if (bestDistanceSqr < 999999999.9f && bestDistanceSqr > (500.0f * 500.0f) && !GameSingleton::IsInView(bestPos))
		{
			const en::F32 d = en::Math::Sqrt(bestDistanceSqr);
			const en::Vector2f deltaNormalized = (bestPos - position) / d;
			radarSprite.setPosition(en::toSF(position + deltaNormalized * 65.0f));
			radarSprite.setRotation(deltaNormalized.getPolarAngle() + 90.0f);
			target.draw(radarSprite);
		}
	}

	// Bullets
	static bool bulletInitialized = false;
	static sf::Sprite bulletSprite;
	if (!bulletInitialized)
	{
		bulletSprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("bullets").Get());
		bulletSprite.setOrigin(8.0f, 8.0f);
		bulletInitialized = true;
	}
	const en::U32 bulletSize = static_cast<en::U32>(GameSingleton::mBullets.size());
	for (en::U32 i = 0; i < bulletSize; ++i)
	{
		bulletSprite.setTextureRect(GetItemBulletTextureRect(GameSingleton::mBullets[i].itemID));
		bulletSprite.setPosition(en::toSF(GameSingleton::mBullets[i].position));
		if (GameSingleton::mBullets[i].itemID == ItemID::Shuriken)
		{
			bulletSprite.setRotation(GameSingleton::mBullets[i].rotation + 90.0f + mShurikenRotation);
		}
		else
		{
			bulletSprite.setRotation(GameSingleton::mBullets[i].rotation + 90.0f);
		}
		
		const en::F32 decayRange = 0.2f * DefaultItemRange * GetItemRange(GameSingleton::mBullets[i].itemID);
		if (GameSingleton::mBullets[i].remainingDistance < decayRange)
		{
			en::F32 factor = GameSingleton::mBullets[i].remainingDistance / decayRange;
			bulletSprite.setScale(0.5f + (factor * 0.5f), 0.5f + (factor * 0.5f));
		}
		else
		{
			bulletSprite.setScale(1.0f, 1.0f);
		}
		
		target.draw(bulletSprite);
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
			en::U8 lifeP = (en::U8)(255 * (GameSingleton::mPlayers[i].chicken.life / GameSingleton::mPlayers[i].chicken.lifeMax));
			color.r = 255;
			color.g = lifeP;
			color.b = lifeP;
		}
		chickenBodySprite.setColor(en::toSF(color));
		chickenBodySprite.setPosition(en::toSF(GameSingleton::mPlayers[i].chicken.position));
		target.draw(chickenBodySprite);
		GameSingleton::mPlayers[i].UpdateSprite();
		target.draw(GameSingleton::mPlayers[i].sprite);
	}

	// Cursor
	GameSingleton::mCursor.setPosition(en::toSF(GameSingleton::mApplication->GetWindow().getCursorPositionView(GameSingleton::mView)));
	GameSingleton::mCursor.setTextureRect(sf::IntRect(32, 0, 32, 32));
	target.draw(GameSingleton::mCursor);

	target.setView(previousView);

	static bool textInitialized = false;
	static sf::Text text;
	if (!textInitialized)
	{
		text.setFont(en::ResourceManager::GetInstance().Get<en::Font>("MainFont").Get());
		text.setCharacterSize(24);
		text.setPosition(1024.0f - 230.0f, 10.0f);
		text.setFillColor(sf::Color::White);
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(1.0f);
		textInitialized = true;
	}
	text.setString("Online players: " + std::to_string(GameSingleton::mPlayers.size()));
	target.draw(text);
}
