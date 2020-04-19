#include "Common.hpp"

#include <Enlivengine/Math/Random.hpp>

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
	case ItemID::M16: return en::seconds(0.5f);
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
	case ItemID::M16: return DefaultItemRange * 1.1f;
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
	case ItemID::M16: return 0.75f;
	default: break;
	}
	return 1.0f;
}

ItemID GetRandomAttackItem()
{
	return static_cast<ItemID>(en::Random::get<en::U32>(static_cast<en::U32>(ItemID::None) + 1, static_cast<en::U32>(ItemID::Count)));
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
	case ItemID::M16: return "m16_soundtrack";
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
	case ItemID::M16: return "m16_loot";
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
	case ItemID::M16: return "m16_fire";
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
	case ItemID::M16: return "m16_hit";
	default: break;
	}
	return "";
}

sf::Packet& operator <<(sf::Packet& packet, const Chicken& chicken)
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

sf::Packet& operator >>(sf::Packet& packet, Chicken& chicken)
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
