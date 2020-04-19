#include "Common.hpp"

#include <Enlivengine/Math/Random.hpp>

const char* GetRejectReasonString(RejectReason rejectReason)
{
	switch (rejectReason)
	{
	case RejectReason::UnknownError: break;
	case RejectReason::TooManyPlayers: return "TooManyPlayers"; break;
	case RejectReason::Blacklisted: return "Blacklisted"; break;
	case RejectReason::Kicked: return "Kicked"; break;
	case RejectReason::Timeout: return "Timeout"; break;
	case RejectReason::Banned: return "Banned"; break;
	default: break;
	}
	return "<Unknown>";
}

bool IsValidItemForAttack(ItemID itemID)
{
	return itemID > ItemID::None && itemID < ItemID::Count;
}

en::Time GetItemCooldown(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return en::seconds(3600.0f); 
	case ItemID::Shuriken: return en::seconds(1.0f);
	case ItemID::Laser: return en::seconds(0.5f);
	case ItemID::Crossbow: return en::seconds(2.0f);
	case ItemID::Uzi: return en::seconds(0.33f);
	//case ItemID::M16: return en::seconds(0.5f);
	default: break;
	}
	return en::seconds(3600.0f);
}

en::F32 GetItemRange(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return 0.0f;
	case ItemID::Shuriken: return DefaultItemRange * 0.7f;
	case ItemID::Laser: return DefaultItemRange * 1.25f;
	case ItemID::Crossbow: return DefaultItemRange * 1.15f;
	case ItemID::Uzi: return DefaultItemRange * 0.9f;
		//case ItemID::M16: return DefaultItemRange * 1.1f;
	default: break;
	}
	return 0.0f;
}

en::F32 GetItemWeight(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return 1.0f;
	case ItemID::Shuriken: return 1.25f;
	case ItemID::Laser: return 0.5f;
	case ItemID::Crossbow: return 0.8f;
	case ItemID::Uzi: return 0.9f;
		//case ItemID::M16: return 0.75f;
	default: break;
	}
	return 1.0f;
}

ItemID GetRandomAttackItem()
{
	return static_cast<ItemID>(en::Random::get<en::U32>(static_cast<en::U32>(ItemID::None) + 1, static_cast<en::U32>(ItemID::Crossbow)));
}

const char* GetItemTextureName(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return "chicken_vanilla";
	case ItemID::Shuriken: return "chicken_shuriken";
	case ItemID::Laser: return "chicken_laser";
	case ItemID::Crossbow: return "chicken_crossbow";
	case ItemID::Uzi: return "chicken_uzi";
		//case ItemID::M16: return "chicken_m16";
	default: break;
	}
	return "chicken_vanilla";
}

const char* GetItemMusicName(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return ""; // TODO : ?
	case ItemID::Shuriken: return "shuriken_soundtrack";
	case ItemID::Laser: return "laser_soundtrack";
	case ItemID::Crossbow: return "crossbow_soundtrack";
	case ItemID::Uzi: return "uzi_soundtrack";
		//case ItemID::M16: return "m16_soundtrack";
	default: break;
	}
	return "";
}

const char* GetItemSoundLootName(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return ""; // TODO : ?
	case ItemID::Shuriken: return "shuriken_loot";
	case ItemID::Laser: return "laser_loot";
	case ItemID::Crossbow: return "crossbow_loot";
	case ItemID::Uzi: return "uzi_loot";
		//case ItemID::M16: return "m16_loot";
	default: break;
	}
	return "";
}

const char* GetItemSoundFireName(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return ""; // TODO : ?
	case ItemID::Shuriken: return "shuriken_fire";
	case ItemID::Laser: return "laser_fire";
	case ItemID::Crossbow: return "crossbow_fire";
	case ItemID::Uzi: return "uzi_fire";
		//case ItemID::M16: return "m16_fire";
	default: break;
	}
	return "";
}

const char* GetItemSoundHitName(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: return ""; // TODO : ?
	case ItemID::Shuriken: return "shuriken_hit";
	case ItemID::Laser: return "laser_hit";
	case ItemID::Crossbow: return "crossbow_hit";
	case ItemID::Uzi: return "uzi_hit";
		//case ItemID::M16: return "m16_hit";
	default: break;
	}
	return "";
}

sf::IntRect GetItemLootTextureRect(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: break;
	case ItemID::Shuriken: return sf::IntRect(0, 0, 32, 32);
	case ItemID::Laser: return sf::IntRect(32, 0, 32, 32);
	case ItemID::Crossbow: return sf::IntRect(64, 0, 32, 32);
	case ItemID::Uzi: return sf::IntRect(96, 0, 32, 32);
		//case ItemID::M16: return sf::IntRect(128, 0, 32, 32);
	default: break;
	}
	return sf::IntRect(0, 0, 0, 0);
}

sf::IntRect GetItemBulletTextureRect(ItemID itemID)
{
	switch (itemID)
	{
	case ItemID::None: break;
	case ItemID::Shuriken: return sf::IntRect(0, 0, 16, 16);
	case ItemID::Laser: return sf::IntRect(16, 0, 16, 16);
	case ItemID::Crossbow: return sf::IntRect(32, 0, 16, 16);
	case ItemID::Uzi: return sf::IntRect(48, 0, 16, 16);
		//case ItemID::M16: return sf::IntRect(64, 0, 16, 16);
	default: break;
	}
	return sf::IntRect(0, 0, 0, 0);
}

sf::Packet& operator<<(sf::Packet& packet, const Chicken& chicken)
{
	packet << chicken.position.x;
	packet << chicken.position.y;
	packet << chicken.rotation;
	packet << static_cast<en::U32>(chicken.itemID);
	packet << chicken.lifeMax;
	packet << chicken.life;
	packet << chicken.speed;
	packet << chicken.attack;
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, Chicken& chicken)
{
	en::U32 itemIDRaw;
	packet >> chicken.position.x;
	packet >> chicken.position.y;
	packet >> chicken.rotation;
	packet >> itemIDRaw;
	packet >> chicken.lifeMax;
	packet >> chicken.speed;
	packet >> chicken.attack;
	chicken.itemID = static_cast<ItemID>(itemIDRaw);
	return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const Seed& seed)
{
	packet << seed.seedUID;
	packet << seed.position.x;
	packet << seed.position.y;
	packet << seed.clientID;
	packet << static_cast<en::F32>(seed.remainingTime.asSeconds());
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, Seed& seed)
{
	en::F32 seconds;
	packet >> seed.seedUID;
	packet >> seed.position.x;
	packet >> seed.position.y;
	packet >> seed.clientID;
	packet >> seconds;
	seed.remainingTime = en::seconds(seconds);
	return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const Item& item)
{
	packet << item.itemUID;
	packet << static_cast<en::U32>(item.itemID);
	packet << item.position.x;
	packet << item.position.y;
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, Item& item)
{
	en::U32 itemIDRaw;
	packet >> item.itemUID;
	packet >> itemIDRaw;
	packet >> item.position.x;
	packet >> item.position.y;
	item.itemID = static_cast<ItemID>(itemIDRaw);
	return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const Bullet& bullet)
{
	packet << bullet.bulletUID;
	packet << bullet.position.x;
	packet << bullet.position.y;
	packet << bullet.rotation;
	packet << bullet.clientID;
	packet << static_cast<en::U32>(bullet.itemID);
	packet << bullet.remainingDistance;
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, Bullet& bullet)
{
	en::U32 itemIDRaw;
	packet >> bullet.bulletUID;
	packet >> bullet.position.x;
	packet >> bullet.position.y;
	packet >> bullet.rotation;
	packet >> bullet.clientID;
	packet >> itemIDRaw;
	packet >> bullet.remainingDistance;
	bullet.itemID = static_cast<ItemID>(itemIDRaw);
	return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const Blood& blood)
{
	packet << blood.bloodUID;
	packet << blood.position.x;
	packet << blood.position.y;
	packet << static_cast<en::F32>(blood.remainingTime.asSeconds());
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, Blood& blood)
{
	en::F32 seconds;
	packet >> blood.bloodUID;
	packet >> blood.position.x;
	packet >> blood.position.y;
	packet >> seconds;
	blood.remainingTime = en::seconds(seconds);
	return packet;
}

bool Bullet::Update(en::F32 dtSeconds)
{
	const en::F32 distance = dtSeconds * DefaultProjectileSpeed;
	position += en::Vector2f::polar(rotation, distance);
	remainingDistance -= distance;
	if (remainingDistance <= 0.0f)
	{
		return true;
	}
	else
	{
		return false;
	}
}
