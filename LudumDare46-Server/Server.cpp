#include "Server.hpp"

#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/System/Hash.hpp>
#include <Enlivengine/System/Time.hpp>
#include <Enlivengine/Math/Random.hpp>
#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/Application/PathManager.hpp>

#include <SFML/Network.hpp>
#include <vector>

#include <Common.hpp>
#include "Player.hpp"
#include "ServerSocket.hpp"

Server::Server()
	: mSocket()
	, mRunning(false)
	, mMapLoaded(false)
	, mMap()
	, mMapSize(2000.0f, 2000.0f)
	, mPlayers()
	, mSeeds()
	, mItems()
	, mBullets()
{
}

bool Server::Start(int argc, char** argv)
{
	en::LogManager::GetInstance().Initialize();

	if (argc >= 1)
	{
		en::PathManager::GetInstance().SetExecutablePath(argv[0]);
		std::string a = en::PathManager::GetInstance().GetAssetsPath();
	}

	mSocket.SetSocketPort((argc >= 2) ? static_cast<en::U16>(std::atoi(argv[1])) : DefaultServerPort);
	if (!mSocket.Start())
	{
		return false;
	}

	if (mMap.LoadFromFile(DefaultServerMapPath))
	{
		mMapLoaded = true;
		mMapSize.x = 1.0f * mMap.GetSize().x * mMap.GetTileSize().x;
		mMapSize.y = 1.0f * mMap.GetSize().y * mMap.GetTileSize().y;
	}
	else
	{
		mMapLoaded = false;
		mMapSize.x = DefaultMapSizeX;
		mMapSize.y = DefaultMapSizeY;
	}

	mRunning = true;

	return true;
}

void Server::Stop()
{
	mRunning = false;

	mSocket.SetBlocking(true);
	sf::Packet stoppingPacket;
	stoppingPacket << static_cast<en::U8>(ServerPacketID::Stopping);
	SendToAllPlayers(stoppingPacket);

	mSocket.Stop();
}

bool Server::Run()
{
	en::Clock clock;
	en::Time stepTime = en::Time::Zero;
	en::Time tickTime = en::Time::Zero;

	const en::Time stepInterval = DefaultStepInterval;
	const en::Time tickInterval = DefaultTickInterval;
	while (IsRunning())
	{
		const en::Time dt = clock.restart();
		stepTime += dt;
		tickTime += dt;

		HandleIncomingPackets();

		while (stepTime >= stepInterval)
		{
			UpdateLogic(stepInterval);
			stepTime -= stepInterval;
		}

		while (tickTime >= tickInterval)
		{
			Tick(tickInterval);
			tickTime -= tickInterval;
		}
	}
	return true;
}

bool Server::IsRunning() const
{
	return mRunning;
}

void Server::UpdateLogic(en::Time dt)
{
	const en::F32 dtSeconds = dt.asSeconds();

	const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		mPlayers[i].lastPacketTime += dt;

		UpdatePlayer(dtSeconds, mPlayers[i]);
	}

	static en::U32 optim = 0;
	optim++;
	en::U32 c = optim % 3;
	if (c == 0)
	{
		UpdateBullets(dt);
	}
	else if (c == 1)
	{
		UpdateLoots(dt);
	}
	else
	{
		// 
	}
}

void Server::Tick(en::Time dt)
{
	en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; )
	{
		if (mPlayers[i].needUpdate)
		{
			SendUpdateChickenPacket(mPlayers[i].clientID, mPlayers[i].chicken);
			mPlayers[i].needUpdate = false;
		}

		// Timeout detection
		if (mPlayers[i].lastPacketTime > DefaultServerTimeout)
		{
			SendConnectionRejectedPacket(mPlayers[i].remoteAddress, mPlayers[i].remotePort, RejectReason::Timeout);
			SendClientLeftPacket(mPlayers[i].clientID);
			mPlayers.erase(mPlayers.begin() + i);
			size--;
		}
		else
		{
			i++;
		}
	}
}

void Server::HandleIncomingPackets()
{
	sf::Packet receivedPacket;
	sf::IpAddress remoteAddress;
	en::U16 remotePort;
	while (mSocket.PollPacket(receivedPacket, remoteAddress, remotePort))
	{
		bool ignorePacket = false;

		en::U8 packetIDRaw;
		receivedPacket >> packetIDRaw;
		if (packetIDRaw >= static_cast<en::U8>(ClientPacketID::Count))
		{
			ignorePacket = true;
		}

		UpdateLastPacketTime(remoteAddress, remotePort);

		const ClientPacketID packetID = static_cast<ClientPacketID>(packetIDRaw);
		switch (packetID)
		{
		case ClientPacketID::Ping:
		{
			LogInfo(en::LogChannel::All, 5, "Ping%s", "");
			SendPongPacket(remoteAddress, remotePort);
		} break;
		case ClientPacketID::Pong:
		{
			LogInfo(en::LogChannel::All, 5, "Pong%s", "");
		} break;
		case ClientPacketID::Join:
		{
			const en::U32 clientID = GenerateClientID(remoteAddress, remotePort);
			const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
			const bool slotAvailable = (mPlayers.size() < DefaultMaxPlayers);
			const bool blacklisted = IsBlacklisted(remoteAddress);
			if (slotAvailable && !blacklisted && playerIndex == -1)
			{
				LogInfo(en::LogChannel::All, 5, "Player joined ClientID %d from %s:%d", clientID, remoteAddress.toString().c_str(), remotePort);

				Player newPlayer;
				newPlayer.remoteAddress = remoteAddress;
				newPlayer.remotePort = remotePort;
				newPlayer.clientID = clientID;
				newPlayer.lastPacketTime = en::Time::Zero;
				newPlayer.nickname = "Player" + std::to_string(clientID); // TODO : nickname
				newPlayer.chicken.position = GetRandomPositionSpawn();
				newPlayer.chicken.rotation = 0.0f;
				newPlayer.chicken.itemID = ItemID::None;
				newPlayer.chicken.lifeMax = DefaultChickenLife;
				newPlayer.chicken.life = DefaultChickenLife;
				newPlayer.chicken.speed = DefaultChickenSpeed;
				newPlayer.chicken.attack = DefaultChickenAttack;
				newPlayer.cooldown = en::Time::Zero;
				newPlayer.state = PlayingState::Playing;

				SendConnectionAcceptedPacket(remoteAddress, remotePort, clientID);

				SendServerInfo(remoteAddress, remotePort);
				const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
				for (en::U32 i = 0; i < playerSize; ++i)
				{
					SendPlayerInfo(remoteAddress, remotePort, mPlayers[i]);
				}
				const en::U32 itemSize = static_cast<en::U32>(mItems.size());
				for (en::U32 i = 0; i < itemSize; ++i)
				{
					SendItemInfo(remoteAddress, remotePort, mItems[i]);
				}

				mPlayers.push_back(newPlayer);

				SendClientJoinedPacket(clientID, newPlayer.nickname, newPlayer.chicken);
			}
			else
			{
				if (!slotAvailable)
				{
					LogInfo(en::LogChannel::All, 5, "Player can't join because maximum reached %s:%d", remoteAddress.toString().c_str(), remotePort);
					SendConnectionRejectedPacket(remoteAddress, remotePort, RejectReason::TooManyPlayers);
				}
				else if (blacklisted)
				{
					LogInfo(en::LogChannel::All, 5, "Player can't join because banned %s:%d", remoteAddress.toString().c_str(), remotePort);
					SendConnectionRejectedPacket(remoteAddress, remotePort, RejectReason::Blacklisted);
				}
				else
				{
					// Might be :
					// - Already connected
					LogInfo(en::LogChannel::All, 5, "Player can't join for unknown reason %s:%d", remoteAddress.toString().c_str(), remotePort);
					SendConnectionRejectedPacket(remoteAddress, remotePort, RejectReason::UnknownError);
				}
			}

		} break;
		case ClientPacketID::Leave:
		{
			en::I32 playerIndex = -1;
			const en::U32 clientID = GetClientIDFromIpAddress(remoteAddress, remotePort, &playerIndex);
			if (clientID != en::U32_Max)
			{
				LogInfo(en::LogChannel::All, 5, "Player left ClientID %d from %s:%d", clientID, remoteAddress.toString().c_str(), remotePort);
				SendClientLeftPacket(clientID);
				if (playerIndex >= 0)
				{
					mPlayers.erase(mPlayers.begin() + playerIndex);
				}
			}
			else
			{
				LogInfo(en::LogChannel::All, 5, "Unknown player left from %s:%d", remoteAddress.toString().c_str(), remotePort);
				ignorePacket = true;
			}
		} break;

		case ClientPacketID::DropSeed:
		{
			en::U32 clientID;
			en::Vector2f position;
			receivedPacket >> clientID >> position.x >> position.y;
			if (IsClientIDAssignedToThisPlayer(clientID, remoteAddress, remotePort))
			{
				const en::I32 playerIndex = GetPlayerIndexFromClientID(clientID);
				if (playerIndex >= 0)
				{
					LogInfo(en::LogChannel::All, 2, "Player %d dropped seed at %f %f", mPlayers[playerIndex].clientID, position.x, position.y);
					AddNewSeed(position, clientID);
				}
				else
				{
					ignorePacket = true;
				}
			}
			else
			{
				ignorePacket = true;
			}
		} break;

		default:
		{
			LogWarning(en::LogChannel::All, 6, "Unknown ClientPacketID %d received from %s:%d", packetIDRaw, remoteAddress.toString().c_str(), remotePort);
			ignorePacket = true;
		} break;
		}

		if (ignorePacket)
		{
			LogWarning(en::LogChannel::All, 4, "Ignore packet %d from %s:%d", packetIDRaw, remoteAddress.toString().c_str(), remotePort);
		}
	}
}

void Server::UpdatePlayer(en::F32 dtSeconds, Player& player)
{
	// Select best seed
	en::I32 bestSeedIndex = -1;
	en::F32 bestSeedDistanceSqr = 999999.0f;
	en::Vector2f bestDeltaSeed;
	const en::U32 seedSize = static_cast<en::U32>(mSeeds.size());
	for (en::U32 i = 0; i < seedSize; ++i)
	{
		const en::Vector2f deltaSeed = mSeeds[i].position - player.chicken.position;
		en::F32 distanceSqr = deltaSeed.getSquaredLength();
		if (distanceSqr > DefaultSeedImpactDistanceSqr)
		{
			continue;
		}
		if (player.clientID == mSeeds[i].clientID)
		{
			distanceSqr *= DefaultOwnerPriority;
		}
		else
		{
			distanceSqr *= 9999.0f * DefaultNonOwnerPriority;
		}
		if (distanceSqr < bestSeedDistanceSqr)
		{
			bestSeedDistanceSqr = distanceSqr;
			bestSeedIndex = static_cast<en::I32>(i);
			bestDeltaSeed = deltaSeed;
		}
	}
	
	// Select best target
	en::I32 bestTargetIndex = -1;
	en::F32 bestTargetDistanceSqr = 999999.0f;
	en::Vector2f bestDeltaTarget;
	const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		const Player& otherPlayer = mPlayers[i];
		if (otherPlayer.clientID != player.clientID)
		{
			const en::Vector2f deltaTarget = otherPlayer.chicken.position - player.chicken.position;
			const en::F32 distanceSqr = deltaTarget.getSquaredLength();
			if (distanceSqr < bestTargetDistanceSqr && distanceSqr < DefaultTargetDetectionMaxDistanceSqr)
			{
				bestTargetDistanceSqr = distanceSqr;
				bestTargetIndex = static_cast<en::I32>(i);
				bestDeltaTarget = deltaTarget;
			}
		}
	}

	if (bestSeedIndex == -1 && bestTargetIndex == -1)
		return;

	// Rotate
	bool rotated = false;
	const bool focusSeed = bestSeedDistanceSqr * DefaultSeedPrefFactor < bestTargetDistanceSqr;
	en::F32 targetAngle;
	if (focusSeed)
	{
		if (bestDeltaSeed.x == 0.0f)
		{
			bestDeltaSeed.x += 0.001f;
		}
		targetAngle = en::Math::AngleMagnitude(bestDeltaSeed.getPolarAngle());
	}
	else
	{
		if (bestDeltaTarget.x == 0.0f)
		{
			bestDeltaTarget.x += 0.001f;
		}
		targetAngle = en::Math::AngleMagnitude(bestDeltaTarget.getPolarAngle());
	}
	const en::F32 currentAngle = en::Math::AngleMagnitude(player.chicken.rotation);
	const en::F32 angleWithTarget = en::Math::AngleBetween(currentAngle, targetAngle);
	if (angleWithTarget > DefaultIgnoreRotDeg)
	{
		const en::F32 tc = targetAngle - currentAngle;
		const en::F32 ct = currentAngle - targetAngle;
		const en::F32 correctedTC = (tc >= 0.0f) ? tc : tc + 360.0f;
		const en::F32 correctedCT = (ct >= 0.0f) ? ct : ct + 360.0f;
		const en::F32 sign = (correctedTC <= correctedCT) ? 1.0f : -1.0f;
		player.chicken.rotation = en::Math::AngleMagnitude(player.chicken.rotation + sign * DefaultRotDegPerSecond * dtSeconds);
		player.needUpdate = true;
		rotated = true;
	}

	if (focusSeed && bestSeedIndex == -1)
		return;
	if (!focusSeed && bestTargetIndex == -1)
		return;

	if (focusSeed)
	{
		player.chicken.position += en::Vector2f::polar(player.chicken.rotation) * dtSeconds * player.chicken.speed * GetItemWeight(player.chicken.itemID);
		player.needUpdate = true;

		// Eat seed
		const en::Vector2f deltaSeed2 = mSeeds[bestSeedIndex].position - player.chicken.position;
		const en::F32 distanceSqr = deltaSeed2.getSquaredLength();
		if (distanceSqr < DefaultTooCloseDistanceSqr)
		{
			SendRemoveSeedPacket(mSeeds[bestSeedIndex].seedUID, true, player.clientID);
			mSeeds.erase(mSeeds.begin() + bestSeedIndex);
		}
	}

	// Try to shoot bullet
	player.cooldown += en::seconds(dtSeconds);
	if (bestTargetIndex >= 0)
	{
		en::Time cooldown = GetItemCooldown(player.chicken.itemID);
		if (IsValidItemForAttack(player.chicken.itemID) && player.cooldown >= cooldown)
		{
			en::F32 range = GetItemRange(player.chicken.itemID);
			const en::F32 rangeSqr = (range + 15.0f) * (range + 15.0f); // Hack because we don't care exactly the offset in fact
			if (bestTargetDistanceSqr < rangeSqr)
			{
				static const en::F32 cos30 = en::Math::Cos(30.0f);
				const en::Vector2f forward = en::Vector2f::polar(player.chicken.rotation);
				const en::F32 distance = en::Math::Sqrt(bestTargetDistanceSqr);
				const en::Vector2f normalizedDelta = en::Vector2f(bestDeltaTarget.x / distance, bestDeltaTarget.y / distance);
				const en::F32 dotProduct = en::Math::Abs(forward.dotProduct(normalizedDelta));
				LogInfo(en::LogChannel::All, 2, "%f:%f %f, %f %f", player.chicken.rotation, forward.x, forward.y, normalizedDelta.x, normalizedDelta.y);
				LogInfo(en::LogChannel::All, 2, "%d Bullet in range cos30:%f dotProduct:%f", player.clientID, cos30, dotProduct);
				if (dotProduct < cos30)
				{
					LogInfo(en::LogChannel::All, 2, "%d Bullet striked", player.clientID);
					const en::Vector2f rotatedWeaponOffset = en::Vector2f(DefaultWeaponOffset).rotated(player.chicken.rotation);
					player.cooldown = en::Time::Zero;
					AddNewBullet(player.chicken.position + rotatedWeaponOffset, bestDeltaTarget.getPolarAngle(), player.clientID, player.chicken.itemID, range);
				}
			}
		}
	}
}

void Server::UpdateBullets(en::Time dt)
{
	const en::F32 dtSeconds = dt.asSeconds();
	en::U32 bulletSize = static_cast<en::U32>(mBullets.size());
	for (en::U32 i = 0; i < bulletSize; )
	{
		if (mBullets[i].Update(dtSeconds))
		{
			mBullets.erase(mBullets.begin() + i);
			bulletSize--;
		}
		else
		{
			bool hit = false;
			en::U32 size = static_cast<en::U32>(mPlayers.size());
			for (en::U32 j = 0; j < size && !hit; ++j)
			{
				if ((mPlayers[j].chicken.position - mBullets[i].position).getSquaredLength() < 35.0f * 35.0f)
				{
					hit = true;
					SendHitChickenPacket(mPlayers[j].clientID, mBullets[i].clientID);
				}
			}
			if (hit)
			{
				mBullets.erase(mBullets.begin() + i);
				bulletSize--;
			}
			else
			{
				i++;
			}
		}
	}
}

void Server::UpdateLoots(en::Time dt)
{
	static en::Time spawnTimeCounter = en::Time::Zero;
	spawnTimeCounter += dt;
	if (spawnTimeCounter >= DefaultSpawnItemInterval)
	{
		if (mItems.size() >= DefaultMaxItemAmount)
		{
			SendRemoveItemPacket(mItems[0].itemUID, false, en::U32_Max);
			mItems.erase(mItems.begin());
		}
		else
		{
			spawnTimeCounter = en::Time::Zero;
			AddNewItem(GetRandomPositionItem(), GetRandomAttackItem());
		}
	}

	// Item pick up
	{
		en::U32 itemSize = static_cast<en::U32>(mItems.size());
		const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
		for (en::U32 i = 0; i < playerSize; ++i)
		{
			const en::Vector2f pPos = mPlayers[i].chicken.position;
			for (en::U32 j = 0; j < itemSize; )
			{
				const en::Vector2f delta = pPos - mItems[j].position;
				if (delta.getSquaredLength() < DefaultItemPickUpDistanceSqr)
				{
					mPlayers[i].chicken.itemID = mItems[j].itemID;
					mPlayers[i].needUpdate = true;

					SendRemoveItemPacket(mItems[j].itemUID, true, mPlayers[i].clientID);
					mItems.erase(mItems.begin() + j);
					itemSize--;
				}
				else
				{
					j++;
				}
			}
		}
	}
}

void Server::UpdateLastPacketTime(const sf::IpAddress& remoteAddress, en::U16 remotePort)
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		if (mPlayers[i].remotePort == remotePort && mPlayers[i].remoteAddress == remoteAddress)
		{
			mPlayers[i].lastPacketTime = en::Time::Zero;
			return;
		}
	}
}

en::U32 Server::GenerateClientID(const sf::IpAddress& remoteAddress, en::U16 remotePort) const
{
	return en::Hash::CombineHash(en::Hash::CRC32(remoteAddress.toString().c_str()), static_cast<en::U32>(remotePort));
}

bool Server::IsClientIDAssignedToThisPlayer(en::U32 clientID, const sf::IpAddress& remoteAddress, en::U16 remotePort) const
{
	return clientID == GenerateClientID(remoteAddress, remotePort);
}

en::U32 Server::GetClientIDFromIpAddress(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::I32* playerIndex) const
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		if (mPlayers[i].remotePort == remotePort && mPlayers[i].remoteAddress == remoteAddress)
		{
			if (playerIndex != nullptr)
			{
				*playerIndex = static_cast<en::I32>(i);
			}
			return mPlayers[i].clientID;
		}
	}
	if (playerIndex != nullptr)
	{
		*playerIndex = -1;
	}
	return en::U32_Max;
}

en::I32 Server::GetPlayerIndexFromClientID(en::U32 clientID) const
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

en::Vector2f Server::GetRandomPositionSpawn()
{
	if (mMapLoaded)
	{
		// TODO : Avoid water
		mMapSize.x = 1.0f * mMap.GetSize().x * mMap.GetTileSize().x;
		mMapSize.y = 1.0f * mMap.GetSize().y * mMap.GetTileSize().y;
		return { en::Random::get<en::F32>(64.0f, mMapSize.x - 64.0f), en::Random::get<en::F32>(64.0f, mMapSize.y - 64.0f) };
	}
	else
	{
		return { en::Random::get<en::F32>(64.0f, mMapSize.x - 64.0f), en::Random::get<en::F32>(64.0f, mMapSize.y - 64.0f) };
	}
}

en::Vector2f Server::GetRandomPositionItem()
{
	if (mMapLoaded)
	{
		// TODO : Avoid water
		mMapSize.x = 1.0f * mMap.GetSize().x * mMap.GetTileSize().x;
		mMapSize.y = 1.0f * mMap.GetSize().y * mMap.GetTileSize().y;
		return { en::Random::get<en::F32>(64.0f, mMapSize.x - 64.0f), en::Random::get<en::F32>(64.0f, mMapSize.y - 64.0f) };
	}
	else
	{
		return { en::Random::get<en::F32>(64.0f, mMapSize.x - 64.0f), en::Random::get<en::F32>(64.0f, mMapSize.y - 64.0f) };
	}
}

void Server::AddNewSeed(const en::Vector2f& position, en::U32 clientID)
{
	const en::U32 seedSize = static_cast<en::U32>(mSeeds.size());
	for (en::U32 i = 0; i < seedSize; ++i)
	{
		if (mSeeds[i].clientID == clientID)
		{
			SendRemoveSeedPacket(mSeeds[i].seedUID, false, en::U32_Max);
			mSeeds.erase(mSeeds.begin() + i);
			break;
		}
	}

	static en::U32 seedUIDGenerator = 1;
	Seed seed;
	seed.seedUID = seedUIDGenerator++;
	seed.position = position;
	seed.clientID = clientID;
	seed.remainingTime = DefaultSeedLifetime;
	mSeeds.push_back(seed);
	SendAddSeedPacket(seed);
}

void Server::AddNewItem(const en::Vector2f& position, ItemID itemID) 
{
	static en::U32 itemUIDGenerator = 1;
	Item item;
	item.itemUID = itemUIDGenerator++;
	item.itemID = itemID;
	item.position = position;
	mItems.push_back(item);
	SendAddItemPacket(item);
}

void Server::AddNewBullet(const en::Vector2f& position, en::F32 rotation, en::U32 clientID, ItemID itemID, en::F32 remainingDistance)
{
	static en::U32 bulletUIDGenerator = 1;
	Bullet bullet;
	bullet.bulletUID = bulletUIDGenerator++;
	bullet.position = position;
	bullet.rotation = rotation;
	bullet.clientID = clientID;
	bullet.itemID = itemID;
	bullet.remainingDistance = remainingDistance;
	mBullets.push_back(bullet);
	SendShootBulletPacket(bullet);
}

void Server::SendToAllPlayers(sf::Packet& packet)
{
	const en::U32 size = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < size; ++i)
	{
		mSocket.SendPacket(packet, mPlayers[i].remoteAddress, mPlayers[i].remotePort);
	}
}

void Server::SendPingPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::Ping);
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendPongPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::Pong);
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendConnectionAcceptedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, en::U32 clientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ConnectionAccepted);
		packet << clientID;
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendConnectionRejectedPacket(const sf::IpAddress& remoteAddress, en::U16 remotePort, RejectReason reason)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ConnectionRejected);
		packet << static_cast<en::U32>(reason);
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendClientJoinedPacket(en::U32 clientID, const std::string& nickname, const Chicken& chicken)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ClientJoined);
		packet << clientID;
		packet << nickname;
		packet << chicken;
		SendToAllPlayers(packet);
	}
}

void Server::SendClientLeftPacket(en::U32 clientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ClientLeft);
		packet << clientID;
		SendToAllPlayers(packet);
	}
}

void Server::SendServerStopPacket()
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::Stopping);
		mSocket.SetBlocking(true);
		SendToAllPlayers(packet);
		mSocket.SetBlocking(false);
	}
}

void Server::SendServerInfo(const sf::IpAddress& remoteAddress, en::U16 remotePort)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ServerInfo);
		packet << mMapSize.x;
		packet << mMapSize.y;
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendPlayerInfo(const sf::IpAddress& remoteAddress, en::U16 remotePort, const Player& player)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::PlayerInfo);
		packet << player.clientID;
		packet << player.nickname;
		packet << player.chicken;
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendItemInfo(const sf::IpAddress& remoteAddress, en::U16 remotePort, const Item& item)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::AddItem);
		packet << item;
		mSocket.SendPacket(packet, remoteAddress, remotePort);
	}
}

void Server::SendUpdateChickenPacket(en::U32 clientID, const Chicken& chicken)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::UpdateChicken);
		packet << clientID;
		packet << chicken;
		SendToAllPlayers(packet);
	}
}

void Server::SendAddSeedPacket(const Seed& seed)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::AddSeed);
		packet << seed;
		SendToAllPlayers(packet);
	}
}

void Server::SendRemoveSeedPacket(en::U32 seedUID, bool eated, en::U32 eaterClientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::RemoveSeed);
		packet << seedUID;
		packet << eated;
		packet << eaterClientID;
		SendToAllPlayers(packet);
	}
}

void Server::SendAddItemPacket(const Item& item)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::AddItem);
		packet << item;
		SendToAllPlayers(packet);
	}
}

void Server::SendRemoveItemPacket(en::U32 itemUID, bool pickedUp, en::U32 pickerClientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::RemoveItem);
		packet << itemUID;
		packet << pickedUp;
		packet << pickerClientID;
		SendToAllPlayers(packet);
	}
}

void Server::SendShootBulletPacket(const Bullet& bullet)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ShootBullet);
		packet << bullet;
		SendToAllPlayers(packet);
	}
}

void Server::SendHitChickenPacket(en::U32 clientID, en::U32 killerClientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::HitChicken);
		packet << clientID;
		packet << killerClientID;
		SendToAllPlayers(packet);
	}
}

void Server::SendKillChickenPacket(en::U32 clientID, en::U32 killerClientID)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::KillChicken);
		packet << clientID;
		packet << killerClientID;
		SendToAllPlayers(packet);
	}
}

void Server::SendRespawnChickenPacket(en::U32 clientID, const en::Vector2f& position)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::RespawnChicken);
		packet << clientID;
		packet << position.x;
		packet << position.y;
		SendToAllPlayers(packet);
	}
}
