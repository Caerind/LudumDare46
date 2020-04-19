#pragma once

#include <Enlivengine/System/PrimitiveTypes.hpp>
#include <Enlivengine/System/Time.hpp>
#include <Enlivengine/Math/Vector2.hpp>
#include <SFML/Network/Packet.hpp>

// Network
#define DefaultServerAddress "92.222.79.62"
#define DefaultServerPort 3457

// Server
#define DefaultTimeout en::seconds(60.0f)
#define DefaultStepInterval en::seconds(1.0f / 60.f)
#define DefaultTickInterval en::seconds(1.0f / 20.f)
#define DefaultSleepTime sf::seconds(1.0f / 5.f)
#define DefaultMaxPlayers 16

// Movement
#define DefaultSeedInterval en::seconds(0.3f)
#define DefaultSeedLifetime en::seconds(3.0f)
#define DefaultSeedImpactDistance 400.0f
#define DefaultSeedImpactDistanceSqr 400.0f * 400.0f
#define DefaultTooCloseDistance 30.0f
#define DefaultTooCloseDistanceSqr 30.0f * 30.0f
#define DefaultChickenAvoidanceMinDistance 200.0f
#define DefaultChickenAvoidanceMinDistanceSqr 200.0f * 200.0f
#define DefaultOwnerPriority 0.5f
#define DefaultNonOwnerPriority 1.0f
#define DefaultWeaponOffset 20.0f, -22.0f

// Stats
#define DefaultChickenLife 100.0f
#define DefaultChickenSpeed 100.0f
#define DefaultChickenAttack 20.0f
#define DefaultProjectileSpeed 300.0f

// Server -> Client
enum class ServerPacketID : en::U8
{ 
	Ping = 0, // Request a ClientPacketID::Pong
	Pong, // Reply to ClientPacketID::Ping
	ConnectionAccepted,
	ConnectionRejected,
	ClientJoined,
	ClientLeft,
	Stopping,

	// Game specific packets
	PlayerInfo,
	UpdateChicken,
	CancelSeed,
	AddSeed,
	RemoveSeed,
	AddItem,
	RemoveItem,
	ShootBullet,

	// Last packet ID
	Count
};
static_assert(static_cast<en::U32>(ServerPacketID::Count) < 255);

// Client -> Server
enum class ClientPacketID : en::U8
{
	Ping = 0, // "Just want the server to send back a Pong"
	Pong, // "Just sending back a Pong when server asked for Ping"
	Join, // "I want to join the server please"
	Leave, // "I'm leaving the server"

	// Game specific packet
	DropSeed,

	// Last packet ID
	Count
};
static_assert(static_cast<en::U32>(ClientPacketID::Count) < 255);


enum class RejectReason : en::U8
{
	UnknownError = 0,
	TooManyPlayers,
	Blacklisted,
	Kicked,
	Timeout,
	Banned,

	Count
};

enum class ItemID : en::U32
{
	None,
	Shuriken,
	Laser,

	Count
};

struct Chicken
{
	en::Vector2f position;
	en::F32 rotation;
	ItemID itemID;

	en::F32 lifeMax;
	en::F32 life;
	en::F32 speed;
	en::F32 attack;
};

sf::Packet& operator <<(sf::Packet& packet, const Chicken& chicken);
sf::Packet& operator >>(sf::Packet& packet, Chicken& chicken);