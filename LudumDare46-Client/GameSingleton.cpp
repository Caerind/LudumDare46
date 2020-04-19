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
std::vector<en::Vector2f> GameSingleton::mCancelSeeds;
en::Application::onApplicationStoppedType::ConnectionGuard GameSingleton::mApplicationStoppedSlot; 
sf::Sprite GameSingleton::mCursor;
en::SoundID GameSingleton::mPlayerMovementSoundID;

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
			if (!mClient.IsConnected())
			{
				en::U32 clientID;
				packet >> clientID;
				LogInfo(en::LogChannel::All, 5, "ConnectionAccepted, ClientID %d", clientID);
				mClient.SetClientID(clientID);
			}
			else
			{
				LogError(en::LogChannel::All, 9, "Already connected %d", mClient.GetClientID());
			}
		} break;
		case ServerPacketID::ConnectionRejected:
		{
			en::U32 rejectReason;
			packet >> rejectReason;
			LogInfo(en::LogChannel::All, 5, "ConnectionRejected %d", rejectReason);
			mClient.SetClientID(en::U32_Max);
			mClient.Stop();
		} break;
		case ServerPacketID::ClientJoined:
		{
			en::U32 clientID;
			std::string nickname;
			Chicken chicken;
			packet >> clientID >> nickname >> chicken;
			LogInfo(en::LogChannel::All, 5, "ClientJoined, ClientID %d, Nickname %s", clientID, nickname.c_str());
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex == -1)
			{
				Player newPlayer;
				newPlayer.clientID = clientID;
				newPlayer.nickname = nickname;
				newPlayer.chicken = chicken;
				newPlayer.sprite.setOrigin({32.0f, 32.0f});
				newPlayer.UpdateSprite();
				mPlayers.push_back(newPlayer);
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Client already joined%s", "");
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
			else
			{
				LogWarning(en::LogChannel::All, 6, "Client already left or not logged%s", "");
			}
		} break;
		case ServerPacketID::Stopping:
		{
			LogInfo(en::LogChannel::All, 5, "ServerStopping%s", "");
			mClient.SetClientID(en::U32_Max);
			mClient.Stop();
		} break;
		case ServerPacketID::PlayerInfo:
		{
			en::U32 clientID;
			std::string nickname;
			Chicken chicken;
			packet >> clientID >> nickname >> chicken;
			LogInfo(en::LogChannel::All, 5, "PlayerInfo, ClientID %d, Nickname %s", clientID, nickname.c_str());
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex == -1)
			{
				Player newPlayer;
				newPlayer.clientID = clientID;
				newPlayer.nickname = nickname;
				newPlayer.chicken = chicken;
				newPlayer.sprite.setOrigin({ 32.0f, 32.0f });
				newPlayer.UpdateSprite();
				mPlayers.push_back(newPlayer);
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Player info already herer%s", "");
			}
		} break;
		case ServerPacketID::UpdateChicken:
		{
			en::U32 clientID;
			packet >> clientID;
			LogInfo(en::LogChannel::All, 5, "PlayerPosition, ClientID %d", clientID);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			if (playerIndex >= 0)
			{
				ItemID previousItemID = mPlayers[playerIndex].chicken.itemID;
				en::F32 previousLife = mPlayers[playerIndex].chicken.life;
				packet >> mPlayers[playerIndex].chicken;
				if (previousItemID != mPlayers[playerIndex].chicken.itemID)
				{
					if (mPlayers[playerIndex].clientID == mClient.GetClientID())
					{
						switch (mPlayers[playerIndex].chicken.itemID)
						{
						case ItemID::Shuriken: en::AudioSystem::GetInstance().PlaySound("shuriken_loot"); break; // TODO : Move out
						case ItemID::Laser: en::AudioSystem::GetInstance().PlaySound("shuriken_loot"); break; // TODO : Move out
						default: break;
						}
					}
				}
				if (previousLife > mPlayers[playerIndex].chicken.life)
				{
					en::AudioSystem::GetInstance().PlaySound("chicken_damage");

					Blood blood;
					blood.position = mPlayers[playerIndex].chicken.position;
					blood.position.x += en::Random::get<en::F32>(-20.0f, 20.0f); // TODO : Move out
					blood.position.y += en::Random::get<en::F32>(-20.0f, 20.0f); // TODO : Move out
					blood.delay = en::Time::Zero;
					blood.index = en::Random::get<en::U32>(0, 3); // TODO : Move out
					mBloods.push_back(blood);
				}
				mPlayers[playerIndex].UpdateSprite();
			}
			else
			{
				LogWarning(en::LogChannel::All, 6, "Unknown player moved %d", packetIDRaw);
			}
		} break;
		case ServerPacketID::CancelSeed:
		{
			en::Vector2f position;
			packet >> position.x >> position.y;
			LogInfo(en::LogChannel::All, 4, "Cancel seed %f %f", position.x, position.y);
			mCancelSeeds.push_back(position);
		} break;
		case ServerPacketID::AddSeed:
		{
			Seed seed;
			packet >> seed.seedID >> seed.position.x >> seed.position.y;
			LogInfo(en::LogChannel::All, 4, "New seed %d %f %f", seed.seedID, seed.position.x, seed.position.y);
			mSeeds.push_back(seed);
		} break;
		case ServerPacketID::RemoveSeed:
		{
			en::U32 seedID;
			bool eated;
			packet >> seedID >> eated;
			LogInfo(en::LogChannel::All, 4, "Remove seed %d", seedID);
			bool removed = false;
			en::U32 size = static_cast<en::U32>(mSeeds.size());
			for (en::U32 i = 0; i < size && !removed; )
			{
				if (mSeeds[i].seedID == seedID)
				{
					const bool isInView = true; // TODO ; Is in view
					if (eated && isInView)
					{
						// TODO : Play sound
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
			en::U32 itemRaw;
			Item item;
			packet >> item.itemID >> itemRaw >> item.position.x >> item.position.y;
			item.item = static_cast<ItemID>(itemRaw);
			LogInfo(en::LogChannel::All, 4, "New item %d %d %f %f", item.itemID, item.item, item.position.x, item.position.y);
			mItems.push_back(item);
		} break;
		case ServerPacketID::RemoveItem:
		{
			en::U32 itemID;
			bool pickedUp;
			packet >> itemID >> pickedUp;
			LogInfo(en::LogChannel::All, 4, "Remove item %d", itemID);
			bool removed = false;
			en::U32 size = static_cast<en::U32>(mItems.size());
			for (en::U32 i = 0; i < size && !removed; )
			{
				if (mItems[i].itemID == itemID)
				{
					if (pickedUp && IsInView(mItems[i].position))
					{
						en::SoundPtr sound;
						switch (mItems[i].item)
						{
						case ItemID::Shuriken: sound = en::AudioSystem::GetInstance().PlaySound("shuriken_loot"); break; // TODO : Move out
						case ItemID::Laser: sound = en::AudioSystem::GetInstance().PlaySound("shuriken_loot"); break; // TODO : Move out
						default: break;
						}
						if (sound.IsValid())
						{
							sound.SetVolume(0.25f);
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
			en::U32 itemIDRaw;
			Bullet bullet;
			packet >> bullet.position.x >> bullet.position.y >> itemIDRaw >> bullet.remainingDistance;
			LogInfo(en::LogChannel::All, 4, "Shoot bullet%s", "");
			bullet.itemID = static_cast<ItemID>(itemIDRaw);
			mBullets.push_back(bullet);
		} break;

		default:
		{
			LogWarning(en::LogChannel::All, 6, "Unknown ServerPacketID %d received", packetIDRaw);
		} break;
		}
	}
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
