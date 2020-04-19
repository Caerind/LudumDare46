#include "GameSingleton.hpp"

#include <Enlivengine/Application/ResourceManager.hpp>
#include <Enlivengine/Graphics/SFMLResources.hpp>

#include <Common.hpp>

en::Application* GameSingleton::mApplication;
ClientSocket GameSingleton::mClient;
en::Time GameSingleton::mLastPacketTime;
GameMap GameSingleton::mMap;
en::View GameSingleton::mView;
std::vector<Player> GameSingleton::mPlayers;
std::vector<Seed> GameSingleton::mSeeds;
std::vector<Item> GameSingleton::mItems;
std::vector<Bullet> GameSingleton::mBullets;
std::vector<Blood> GameSingleton::mBloods;
en::Application::onApplicationStoppedType::ConnectionGuard GameSingleton::mApplicationStoppedSlot; 
sf::Sprite GameSingleton::mCursor;
GameSingleton::PlayingState GameSingleton::mPlayingState;
en::MusicPtr GameSingleton::mMusic;

void GameSingleton::ConnectWindowCloseSlot()
{
	mApplicationStoppedSlot.connect(mApplication->onApplicationStopped, [&](const en::Application*) { SendLeavePacket(); });
}

void GameSingleton::HandleIncomingPackets()
{
	ENLIVE_PROFILE_FUNCTION();

	if (!mClient.IsRunning())
	{
		return;
	}

	sf::Packet packet;
	while (mClient.PollPacket(packet))
	{
		mLastPacketTime = en::Time::Zero;

		en::U8 packetIDRaw;
		packet >> packetIDRaw;

		const ServerPacketID packetID = static_cast<ServerPacketID>(packetIDRaw);
		switch (packetID)
		{
		case ServerPacketID::Ping:
		{
			LogInfo(en::LogChannel::All, 5, "Ping%s", "");
			SendPongPacket();
		} break;
		case ServerPacketID::Pong:
		{
			LogInfo(en::LogChannel::All, 5, "Pong%s", "");
		} break;
		case ServerPacketID::ConnectionAccepted:
		{
			en::U32 clientID;
			packet >> clientID;
			LogInfo(en::LogChannel::All, 5, "ConnectionAccepted, ClientID %d", clientID);
			mClient.SetClientID(clientID);
		} break;
		case ServerPacketID::ConnectionRejected:
		{
			en::U32 rejectReason;
			packet >> rejectReason;
			LogInfo(en::LogChannel::All, 5, "ConnectionRejected %s", GetRejectReasonString(static_cast<RejectReason>(rejectReason)));
			mClient.SetClientID(en::U32_Max);
			mClient.Stop();
		} break;
		case ServerPacketID::ClientJoined:
		{
			en::U32 clientID;
			std::string nickname;
			Chicken chicken;
			packet >> clientID >> nickname >> chicken;
			LogInfo(en::LogChannel::All, 5, "%s joined (ID: %d)", nickname.c_str(), clientID);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex == -1)
			{
				Player newPlayer;
				newPlayer.clientID = clientID;
				newPlayer.nickname = nickname;
				newPlayer.chicken = chicken;
				newPlayer.sprite.setOrigin({32.0f, 32.0f});
				mPlayers.push_back(newPlayer);
			}
			else
			{
				// Update player
				Player& newPlayer = mPlayers[playerIndex];
				newPlayer.clientID = clientID;
				newPlayer.nickname = nickname;
				newPlayer.chicken = chicken;
				newPlayer.sprite.setOrigin({ 32.0f, 32.0f });
			}
		} break;
		case ServerPacketID::ClientLeft:
		{
			en::U32 clientID;
			packet >> clientID;
			LogInfo(en::LogChannel::All, 5, "ClientLeft, ClientID %d", clientID);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex >= 0)
			{
				mPlayers.erase(mPlayers.begin() + static_cast<en::U32>(playerIndex));
			}
		} break;
		case ServerPacketID::Stopping:
		{
			LogInfo(en::LogChannel::All, 5, "ServerStopping%s", "");
			mClient.SetClientID(en::U32_Max);
			mClient.Stop();
			mPlayingState = PlayingState::Connecting; // Try to kick when in game state
		} break;
		case ServerPacketID::PlayerInfo:
		{
			en::U32 clientID;
			std::string nickname;
			Chicken chicken;
			packet >> clientID >> nickname >> chicken;
			LogInfo(en::LogChannel::All, 5, "PlayerInfo for %s (ID: %d)", nickname.c_str(), clientID);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex == -1)
			{
				Player newPlayer;
				newPlayer.clientID = clientID;
				newPlayer.nickname = nickname;
				newPlayer.chicken = chicken;
				newPlayer.sprite.setOrigin({ 32.0f, 32.0f });
				mPlayers.push_back(newPlayer);
			}
			else
			{
				// Update player
				Player& newPlayer = mPlayers[playerIndex];
				newPlayer.clientID = clientID;
				newPlayer.nickname = nickname;
				newPlayer.chicken = chicken;
				newPlayer.sprite.setOrigin({ 32.0f, 32.0f });
			}
		} break;
		case ServerPacketID::ItemInfo:
		{
			Item item;
			packet >> item;
			LogInfo(en::LogChannel::All, 4, "New item %d %f %f", item.itemID, item.position.x, item.position.y);
			mItems.push_back(item);
		} break;
		case ServerPacketID::UpdateChicken:
		{
			en::U32 clientID;
			packet >> clientID;
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex >= 0)
			{
				packet >> mPlayers[playerIndex].chicken;
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Unknown player moved %d", packetIDRaw);
			}
		} break;
		case ServerPacketID::AddSeed:
		{
			Seed seed;
			packet >> seed;
			LogInfo(en::LogChannel::All, 4, "New seed %f %f", seed.position.x, seed.position.y);
			mSeeds.push_back(seed);

			if (GameSingleton::IsClient(seed.clientID))
			{
				en::SoundPtr sound = en::AudioSystem::GetInstance().PlaySound("..."); // TODO
				if (sound.IsValid())
				{
					sound.SetVolume(0.25f);
				}
			}
		} break;
		case ServerPacketID::RemoveSeed:
		{
			en::U32 seedUID;
			bool eated;
			en::U32 eaterClientID;
			packet >> seedUID >> eated >> eaterClientID;
			LogInfo(en::LogChannel::All, 4, "Remove seed%s", "");
			bool removed = false;
			en::U32 size = static_cast<en::U32>(mSeeds.size());
			for (en::U32 i = 0; i < size && !removed; )
			{
				if (mSeeds[i].seedUID == seedUID)
				{
					if (eated && IsInView(mSeeds[i].position))
					{
						if (IsClient(eaterClientID))
						{
							en::SoundPtr sound = en::AudioSystem::GetInstance().PlaySound("..."); // TODO
							if (sound.IsValid())
							{
								sound.SetVolume(0.25f);
							}
						}
					}
					mSeeds.erase(mSeeds.begin() + i);
					size--;
					removed = true;
				}
				else
				{
					++i;
				}
			}
		} break;
		case ServerPacketID::AddItem:
		{
			Item item;
			packet >> item;
			LogInfo(en::LogChannel::All, 4, "New item %d %f %f", item.itemID, item.position.x, item.position.y);
			mItems.push_back(item);
		} break;
		case ServerPacketID::RemoveItem:
		{
			en::U32 itemUID;
			bool pickedUp;
			en::U32 pickerClientID;
			packet >> itemUID >> pickedUp >> pickerClientID;
			LogInfo(en::LogChannel::All, 4, "Remove item%s", "");
			bool removed = false;
			en::U32 size = static_cast<en::U32>(mItems.size());
			for (en::U32 i = 0; i < size && !removed; )
			{
				if (mItems[i].itemUID == itemUID)
				{
					if (pickedUp && IsInView(mItems[i].position))
					{
						en::SoundPtr sound = en::AudioSystem::GetInstance().PlaySound(GetItemSoundLootName(mItems[i].itemID));
						if (sound.IsValid())
						{
							sound.SetVolume(0.25f);
						}

						// Play music
						if (IsClient(pickerClientID))
						{
							if (mMusic.IsValid())
							{
								mMusic.Stop();
							}
							mMusic = en::MusicPtr();
							const char* name = GetItemMusicName(GameSingleton::mItems[i].itemID);
							if (name != nullptr && strlen(name) > 0)
							{
								mMusic = en::AudioSystem::GetInstance().PlayMusic(name);
								if (mMusic.IsValid())
								{
									mMusic.SetLoop(true);
									mMusic.SetVolume(0.2f);
								}
							}
						}
					}
					mItems.erase(mItems.begin() + i);
					size--;
					removed = true;
				}
				else
				{
					++i;
				}
			}
		} break;
		case ServerPacketID::ShootBullet:
		{
			Bullet bullet;
			packet >> bullet;
			LogInfo(en::LogChannel::All, 4, "Bullet%s", "");
			mBullets.push_back(bullet);
			
			// Play fire sound
			if (IsInView(bullet.position))
			{
				en::SoundPtr sound = en::AudioSystem::GetInstance().PlaySound(GetItemSoundFireName(bullet.itemID));
				if (sound.IsValid())
				{
					sound.SetVolume(0.25f);
				}
			}
		} break;
		case ServerPacketID::HitChicken:
		{
			en::U32 clientID;
			en::U32 killerClientID;
			packet >> clientID >> killerClientID;
			LogInfo(en::LogChannel::All, 2, "Hit %d", clientID);

			const en::I32 receiverIndex = GetPlayerIndexFromClientID(clientID);
			if (receiverIndex >= 0 && IsInView(mPlayers[receiverIndex].chicken.position))
			{
				en::SoundPtr soundDamage = en::AudioSystem::GetInstance().PlaySound("chicken_damage");
				if (soundDamage.IsValid())
				{
					soundDamage.SetVolume(0.25f);
				}

				const en::I32 killerIndex = GetPlayerIndexFromClientID(killerClientID);
				if (killerIndex >= 0)
				{
					en::SoundPtr soundHit = en::AudioSystem::GetInstance().PlaySound(GetItemSoundHitName(mPlayers[killerIndex].chicken.itemID));
					if (soundHit.IsValid())
					{
						soundHit.SetVolume(0.25f);
					}
				}
			}
		} break;
		case ServerPacketID::KillChicken:
		{
			en::U32 clientID;
			en::U32 killerClientID;
			packet >> clientID >> killerClientID;
			LogInfo(en::LogChannel::All, 2, "Kill %d", clientID);

			const en::I32 receiverIndex = GetPlayerIndexFromClientID(clientID);
			if (receiverIndex >= 0 && IsInView(mPlayers[receiverIndex].chicken.position))
			{
				en::SoundPtr soundDamage = en::AudioSystem::GetInstance().PlaySound("chicken_kill");
				if (soundDamage.IsValid())
				{
					soundDamage.SetVolume(0.25f);
				}
			}

			if (IsClient(killerClientID))
			{
				// TODO : Reward sound
			}

			if (IsClient(clientID))
			{
				// TODO : Die
			}

		} break;
		case ServerPacketID::RespawnChicken:
		{
			// TODO
		} break;

		default:
		{
			LogWarning(en::LogChannel::All, 6, "Unknown ServerPacketID %d received", packetIDRaw);
		} break;
		}
	}
}

bool GameSingleton::HasTimeout(en::Time dt)
{
	if (GameSingleton::mClient.IsConnected())
	{
		GameSingleton::mLastPacketTime += dt;
		if (GameSingleton::mLastPacketTime > DefaultClientTimeout)
		{
			GameSingleton::mClient.SetClientID(en::U32_Max);
			GameSingleton::mClient.Stop();
			return true;
		}
	}
	return false;
}

en::I32 GameSingleton::GetPlayerIndexFromClientID(en::U32 clientID)
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		if (mPlayers[i].clientID == clientID)
		{
			return static_cast<en::I32>(i);
		}
	}
	return -1;
}

bool GameSingleton::IsInView(const en::Vector2f& position)
{
	return GameSingleton::mView.getBounds().contains(position);
}

void GameSingleton::SendPingPacket()
{
	sf::Packet pingPacket;
	pingPacket << static_cast<en::U8>(ClientPacketID::Ping);
	mClient.SendPacket(pingPacket);
}

void GameSingleton::SendPongPacket()
{
	sf::Packet pongPacket;
	pongPacket << static_cast<en::U8>(ClientPacketID::Pong);
	mClient.SendPacket(pongPacket);
}

void GameSingleton::SendJoinPacket()
{
	if (mClient.IsRunning() && !mClient.IsConnected())
	{
		sf::Packet joinPacket;
		joinPacket << static_cast<en::U8>(ClientPacketID::Join);
		mClient.SendPacket(joinPacket);
	}
}

void GameSingleton::SendLeavePacket()
{
	if (mClient.IsRunning() && mClient.IsConnected())
	{
		sf::Packet leavePacket;
		leavePacket << static_cast<en::U8>(ClientPacketID::Leave);
		leavePacket << mClient.GetClientID();
		mClient.SetBlocking(true);
		mClient.SendPacket(leavePacket);
		mClient.SetBlocking(false);
	}
}

void GameSingleton::SendDropSeedPacket(const en::Vector2f& position)
{
	if (mClient.IsRunning() && mClient.IsConnected())
	{
		sf::Packet positionPacket;
		positionPacket << static_cast<en::U8>(ClientPacketID::DropSeed);
		positionPacket << mClient.GetClientID();
		positionPacket << position.x;
		positionPacket << position.y;
		mClient.SendPacket(positionPacket);
	}
}
