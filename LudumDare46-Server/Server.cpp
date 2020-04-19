#include "Server.hpp"

#include <Enlivengine/System/Log.hpp>
#include <Enlivengine/System/Hash.hpp>
#include <Enlivengine/System/Time.hpp>
#include <Enlivengine/Math/Random.hpp>

#include <SFML/Network.hpp>
#include <vector>

#include <Common.hpp>
#include "Player.hpp"
#include "ServerSocket.hpp"

Server::Server()
	: mSocket()
	, mRunning(false)
	, mPlayers()
	, mSeeds()
	, mItems()
	, mBullets()
{
}

bool Server::Start(int argc, char** argv)
{
	mSocket.SetSocketPort((argc >= 2) ? static_cast<en::U16>(std::atoi(argv[1])) : DefaultServerPort);
	if (!mSocket.Start())
	{
		return false;
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

		UpdatePlayerMovement(dtSeconds, mPlayers[i]);
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
		// Update seeds
		en::U32 seedSize = static_cast<en::U32>(mSeeds.size());
		for (en::U32 i = 0; i < seedSize;)
		{
			mSeeds[i].addTime += dt;
			if (mSeeds[i].addTime >= DefaultSeedLifetime)
			{
				SendRemoveSeedPacket(mSeeds[i].seedID, false);
				mSeeds.erase(mSeeds.begin() + i);
				seedSize--;
			}
			else
			{
				i++;
			}
		}
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
				newPlayer.chicken.itemID = ItemID::None; // TODO : Default
				newPlayer.chicken.lifeMax = DefaultChickenLife;
				newPlayer.chicken.life = DefaultChickenLife;
				newPlayer.chicken.speed = DefaultChickenSpeed;
				newPlayer.chicken.attack = DefaultChickenAttack;
				newPlayer.cooldown = en::Time::Zero;
				newPlayer.state = PlayingState::Playing;

				SendConnectionAcceptedPacket(remoteAddress, remotePort, clientID);

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

void Server::UpdatePlayerMovement(en::F32 dtSeconds, Player& player)
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
			distanceSqr *= DefaultNonOwnerPriority;
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
			SendRemoveSeedPacket(mSeeds[bestSeedIndex].seedID, true);
			mSeeds.erase(mSeeds.begin() + bestSeedIndex);
		}
	}






	/*


	// Movement based on seed
	en::Vector2f movement(0.0f, 0.0f);
	if (bestSeedIndex >= 0)
	{
		const Seed& seed = mSeeds[bestSeedIndex];
		const en::Vector2f delta = seed.position - player.chicken.position;
		const en::F32 distanceSqr = delta.getSquaredLength();
		if (distanceSqr < DefaultTooCloseDistanceSqr)
		{
			SendRemoveSeedPacket(seed.seedID, true);
			mSeeds.erase(mSeeds.begin() + bestSeedIndex);
		}
		else if (DefaultTooCloseDistanceSqr < distanceSqr && distanceSqr < DefaultSeedImpactDistanceSqr)
		{
			const en::F32 factor = (DefaultSeedImpactDistanceSqr - distanceSqr) / DefaultSeedImpactDistanceSqr;
			movement += delta * factor;
		}
	}

	// Avoidance
	en::I32 bestTargetIndex = -1;
	en::F32 bestTargetDistanceSqr = 999999.0f;
	const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
	for (en::U32 i = 0; i < playerSize; ++i)
	{
		const Player& otherPlayer = mPlayers[i];
		if (otherPlayer.clientID != player.clientID)
		{
			const en::Vector2f delta = player.chicken.position - otherPlayer.chicken.position;
			const en::F32 distanceSqr = delta.getSquaredLength();

			if (0.01f < distanceSqr && distanceSqr < DefaultChickenAvoidanceMinDistanceSqr)
			{
				const en::F32 factor = (DefaultChickenAvoidanceMinDistanceSqr - distanceSqr) / DefaultChickenAvoidanceMinDistanceSqr;
				movement += delta * factor;
			}
			else if (distanceSqr <= 0.01f)
			{
				player.chicken.position += { 10.0f, 10.0f };
			}

			if (distanceSqr < bestTargetDistanceSqr && distanceSqr < DefaultTargetDetectionMaxDistanceSqr)
			{
				bestTargetDistanceSqr = distanceSqr;
				bestTargetIndex = static_cast<en::I32>(i);
			}
		}
	}

	bool moved = false;
	en::F32 targetAngle;
	const en::F32 currentAngle = en::Math::AngleMagnitude(player.chicken.rotation);

	// Update the movement
	if (movement.getSquaredLength() > 0.0f)
	{
		movement.normalize();
		movement *= dtSeconds * player.chicken.speed * GetItemWeight(player.chicken.itemID);
		player.chicken.position += movement;
		player.needUpdate = true;
		if (en::Math::Equals(movement.x, 0.0f)) // Cheating a bit to avoid 0div
		{ 
			if (movement.x > 0.0f)
			{
				movement.x += 0.01f;
			}
			else
			{
				movement.x -= 0.01f;
			}
		}
		targetAngle = en::Math::AngleMagnitude(movement.getPolarAngle());
		moved = true;
	}
	else
	{
		if (bestTargetIndex >= 0)
		{
			en::Vector2f delta = mPlayers[bestTargetIndex].chicken.position - player.chicken.position;
			if (en::Math::Equals(delta.x, 0.0f)) // Cheating a bit to avoid 0div
			{
				if (delta.x > 0.0f)
				{
					delta.x += 0.01f;
				}
				else
				{
					delta.x -= 0.01f;
				}
			}
			targetAngle = en::Math::AngleMagnitude(delta.getPolarAngle());
		}
		else
		{
			targetAngle = currentAngle;
		}
	}

	{ // Update angle
		if (moved || en::Math::Abs(targetAngle - currentAngle) > 2.0f)
		{
			const en::F32 tc = targetAngle - currentAngle;
			const en::F32 ct = currentAngle - targetAngle;
			const en::F32 correctedTC = (tc >= 0.0f) ? tc : tc + 360.0f;
			const en::F32 correctedCT = (ct >= 0.0f) ? ct : ct + 360.0f;
			const en::F32 sign = (tc <= ct) ? 1.0f : -1.0f;
			player.chicken.rotation += sign * DefaultRotDegPerSecond * dtSeconds;
			player.needUpdate = true;
		}
	}

	*/
}

void Server::UpdateBullets(en::Time dt)
{
	// Try to new shoot bullets
	{
		const en::U32 playerSize = static_cast<en::U32>(mPlayers.size());
		for (en::U32 i = 0; i < playerSize; ++i)
		{
			mPlayers[i].cooldown += dt;
			const ItemID itemID = mPlayers[i].chicken.itemID;
			en::Time cooldown = GetItemCooldown(itemID);
			if (IsValidItemForAttack(itemID) && mPlayers[i].cooldown >= cooldown)
			{
				en::F32 range = GetItemRange(itemID);
				const en::Vector2f forward = en::Vector2f::polar(mPlayers[i].chicken.rotation);
				const en::F32 rangeSqr = (range + 15.0f) * (range + 15.0f); // Hack because we don't care exactly the offset in fact
				en::I32 bestTargetIndex = -1;
				en::F32 bestTargetDistanceSqr = 999999.0f;
				en::F32 shootRotation = 0.0f;
				for (en::U32 j = 0; j < playerSize; ++j)
				{
					if (i != j)
					{
						const en::Vector2f delta = mPlayers[j].chicken.position - mPlayers[i].chicken.position;
						const en::F32 distanceSqr = delta.getSquaredLength();
						if (distanceSqr < rangeSqr && distanceSqr > 0.0f && distanceSqr < bestTargetDistanceSqr)
						{
							const en::F32 distance = en::Math::Sqrt(distanceSqr);
							const en::Vector2f normalizedDelta = en::Vector2f(delta.x / distance, delta.y / distance);
							const en::F32 dotProduct = en::Math::Abs(forward.dotProduct(normalizedDelta));
							static const en::F32 cos30 = en::Math::Cos(30.0f);
							if (dotProduct < cos30)
							{
								bestTargetIndex = static_cast<en::I32>(j);
								bestTargetDistanceSqr = distanceSqr;
								shootRotation = normalizedDelta.getPolarAngle();
							}
						}
					}
				}
				if (bestTargetIndex < 0)
				{
					mPlayers[i].cooldown -= 5 * dt; // Optim : Skip 5 frames
				}
				else
				{
					const en::Vector2f rotatedWeaponOffset = en::Vector2f(DefaultWeaponOffset).rotated(mPlayers[i].chicken.rotation);
					mPlayers[i].cooldown = en::Time::Zero;
					AddNewBullet(mPlayers[i].chicken.position + rotatedWeaponOffset, shootRotation, mPlayers[i].chicken.itemID, range);
				}
			}
		}
	}

	// Update current bullets
	{
		const en::F32 dtSeconds = dt.asSeconds();
		en::U32 bulletSize = static_cast<en::U32>(mBullets.size());
		for (en::U32 i = 0; i < bulletSize; )
		{
			const en::F32 distance = dtSeconds * DefaultProjectileSpeed;
			mBullets[i].position += en::Vector2f::polar(mBullets[i].rotation, distance);
			mBullets[i].remainingDistance -= distance;
			if (mBullets[i].remainingDistance <= 0.0f)
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
			SendRemoveItemPacket(mItems[0].itemID, false);
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
					mPlayers[i].chicken.itemID = mItems[j].item;
					mPlayers[i].needUpdate = true;

					SendRemoveItemPacket(mItems[j].itemID, true);
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
	// TODO : Improve
	static const en::F32 mapSize = 64.0f * 10.0f;
	return { en::Random::get<en::F32>(0.0f, mapSize), en::Random::get<en::F32>(0.0f, mapSize) };
}

en::Vector2f Server::GetRandomPositionItem()
{
	// TODO : Improve
	static const en::F32 mapSize = 64.0f * 10.0f;
	return { en::Random::get<en::F32>(0.0f, mapSize), en::Random::get<en::F32>(0.0f, mapSize) };
}

void Server::AddNewSeed(const en::Vector2f& position, en::U32 clientID)
{
	static en::U32 seedIDGenerator = 1;
	Seed seed;
	seed.seedID = seedIDGenerator++;
	seed.position = position;
	seed.clientID = clientID;
	seed.addTime = en::Time::Zero;
	mSeeds.push_back(seed);
	SendAddSeedPacket(seed);

	const en::U32 seedSize = static_cast<en::U32>(mSeeds.size());
	for (en::U32 i = 0; i < seedSize; ++i)
	{
		if (mSeeds[i].clientID == clientID)
		{
			SendRemoveSeedPacket(mSeeds[i].seedID, false);
			mSeeds.erase(mSeeds.begin() + i);
			break;
		}
	}
}

void Server::AddNewItem(const en::Vector2f& position, ItemID itemID) 
{
	static en::U32 itemIDGenerator = 1;
	Item item;
	item.itemID = itemIDGenerator++;
	item.item = itemID;
	item.position = position;
	mItems.push_back(item);
	SendAddItemPacket(item);
}

void Server::AddNewBullet(const en::Vector2f& position, en::F32 rotation, ItemID itemID, en::F32 remainingDistance)
{
	Bullet bullet;
	bullet.position = position;
	bullet.rotation = rotation;
	bullet.itemID = itemID;
	bullet.remainingDistance = remainingDistance;
	mBullets.push_back(bullet);
	SendShootBulletPacket(position, rotation, itemID, remainingDistance);
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
		packet << item.itemID;
		packet << static_cast<en::U32>(item.item);
		packet << item.position.x;
		packet << item.position.y;
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
		packet << seed.seedID;
		packet << seed.position.x;
		packet << seed.position.y;
		packet << seed.clientID;
		SendToAllPlayers(packet);
	}
}

void Server::SendRemoveSeedPacket(en::U32 seedID, bool eated)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::RemoveSeed);
		packet << seedID;
		packet << eated;
		SendToAllPlayers(packet);
	}
}

void Server::SendAddItemPacket(const Item& item)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::AddItem);
		packet << item.itemID;
		packet << static_cast<en::U32>(item.item);
		packet << item.position.x;
		packet << item.position.y;
		SendToAllPlayers(packet);
	}
}

void Server::SendRemoveItemPacket(en::U32 itemID, bool pickedUp)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::RemoveItem);
		packet << itemID;
		packet << pickedUp;
		SendToAllPlayers(packet);
	}
}

void Server::SendShootBulletPacket(const en::Vector2f& position, en::F32 rotation, ItemID itemID, en::F32 remainingDistance)
{
	if (mSocket.IsRunning())
	{
		sf::Packet packet;
		packet << static_cast<en::U8>(ServerPacketID::ShootBullet);
		packet << position.x;
		packet << position.y;
		packet << rotation;
		packet << static_cast<en::U32>(itemID);
		packet << remainingDistance;
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
