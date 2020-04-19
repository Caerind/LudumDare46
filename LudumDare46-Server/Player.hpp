#pragma once

#include <SFML/Network/IpAddress.hpp>
#include <Enlivengine/Math/Vector2.hpp>
#include <Enlivengine/System/Time.hpp>
#include <Common.hpp>

enum class PlayingState
{
	Playing,
	Respawning
};

struct Player
{
	sf::IpAddress remoteAddress;
	en::U16 remotePort;
	en::U32 clientID;
	en::Time lastPacketTime;

	std::string nickname;
	Chicken chicken;

	en::Time cooldown;
	PlayingState state;
	bool needUpdate;
};

struct Seed
{
	en::U32 seedID;
	en::Vector2f position;
	en::U32 clientID;
	en::Time addTime;
};

struct Item
{
	en::U32 itemID;
	ItemID item;
	en::Vector2f position;
};

struct Bullet
{
	en::Vector2f position;
	en::F32 rotation;
	ItemID itemID;
	en::F32 remainingDistance;
};