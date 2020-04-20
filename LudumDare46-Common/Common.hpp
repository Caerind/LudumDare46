#pragma once

#include <Enlivengine/System/PrimitiveTypes.hpp>
#include <Enlivengine/System/Time.hpp>
#include <Enlivengine/Math/Vector2.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <vector>

// Network
#define DefaultServerAddress "92.222.79.62"
#define DefaultServerPort 3457

// Server
#define DefaultClientTimeout en::seconds(3600.0f)
#define DefaultServerTimeout en::seconds(3600.0f)
#define DefaultStepInterval en::seconds(1.0f / 60.0f)
#define DefaultTickInterval en::seconds(1.0f / 20.0f)
#define DefaultSleepTime sf::seconds(1.0f / 5.0f)
#define DefaultMaxPlayers 16

// Movement
#define DefaultSeedInterval en::seconds(0.3f)
#define DefaultSeedLifetime en::seconds(3.0f)
#define DefaultSeedImpactDistance 400.0f
#define DefaultSeedImpactDistanceSqr 400.0f * 400.0f
#define DefaultSeedPrefFactor 0.9f
#define DefaultItemPickUpDistance 35.0f
#define DefaultItemPickUpDistanceSqr 35.0f * 35.0f
#define DefaultTooCloseDistance 150.0f
#define DefaultTooCloseDistanceSqr 150.0f * 150.0f
#define DefaultChickenAvoidanceMinDistance 200.0f
#define DefaultChickenAvoidanceMinDistanceSqr 200.0f * 200.0f
#define DefaultOwnerPriority 0.6f
#define DefaultNonOwnerPriority 1.0f
#define DefaultWeaponOffset 22.0f, 20.0f
#define DefaultRotDegPerSecond 170.0f
#define DefaultIgnoreRotDeg 4.0f
#define DefaultTargetDetectionMaxDistance 400.0f
#define DefaultTargetDetectionMaxDistanceSqr 400.0f * 400.0f
#define DefaultCameraLerpFactor 0.75f
#define DefaultCameraMaxDistance 500.0f
#define DefaultCameraMaxDistanceSqr 500.0f * 500.0f

// Stats
#define DefaultChickenLife 100.0f
#define DefaultChickenSpeed 160.0f
#define DefaultChickenAttack 20.0f
#define DefaultProjectileSpeed 300.0f
#define DefaultItemRange 400.0f
#define DefaultDetectionRadius 35.0f
#define DefaultDetectionRadiusSqr 35.0f * 35.0f

// MapData
#define DefaultMaxItemAmount 40
#define DefaultSpawnItemInterval en::seconds(0.3f)
#define DefaultMapSizeX 64.0f * 64.0f
#define DefaultMapSizeY 64.0f * 48.0f
#define DefaultMapBorder 64.0f * 6.0f

// Visuals
#define DefaultBloodCount 3
#define DefaultAnimCount 4
#define DefaultShurikenRotDegSpeed 1440.0f
#define DefaultServerMapPath "Assets/Maps/map.tmx"


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
	ServerInfo,
	PlayerInfo,
	ItemInfo,
	UpdateChicken,
	AddSeed,
	RemoveSeed,
	AddItem,
	RemoveItem,
	ShootBullet,
	HitChicken,
	KillChicken,
	RespawnChicken,

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
	Respawn,

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
const char* GetRejectReasonString(RejectReason rejectReason);

enum class ItemID : en::U32
{
	None,
	Shuriken,
	Laser,
	Crossbow,
	Uzi,
	//M16,

	Count
};
bool IsValidItemForAttack(ItemID itemID);
en::Time GetItemCooldown(ItemID itemID);
en::F32 GetItemRange(ItemID itemID);
en::F32 GetItemWeight(ItemID itemID);
ItemID GetRandomAttackItem();
const char* GetItemTextureName(ItemID itemID);
const char* GetItemMusicName(ItemID itemID);
const char* GetItemSoundLootName(ItemID itemID);
const char* GetItemSoundFireName(ItemID itemID);
const char* GetItemSoundHitName(ItemID itemID);
sf::IntRect GetItemLootTextureRect(ItemID itemID);
sf::IntRect GetItemBulletTextureRect(ItemID itemID);

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
sf::Packet& operator<<(sf::Packet& packet, const Chicken& chicken);
sf::Packet& operator>>(sf::Packet& packet, Chicken& chicken);

struct Seed
{
	en::U32 seedUID;
	en::Vector2f position;
	en::U32 clientID;
	en::Time remainingTime;

	static en::I32 GetBestSeedIndex(en::U32 clientID, const en::Vector2f& position, const std::vector<Seed>& seeds, en::Vector2f& delta);
};
sf::Packet& operator<<(sf::Packet& packet, const Seed& seed);
sf::Packet& operator>>(sf::Packet& packet, Seed& seed);

struct Item
{
	en::U32 itemUID;
	ItemID itemID;
	en::Vector2f position;
};
sf::Packet& operator<<(sf::Packet& packet, const Item& item);
sf::Packet& operator>>(sf::Packet& packet, Item& item);

struct Bullet
{
	en::U32 bulletUID;
	en::Vector2f position;
	en::F32 rotation;
	en::U32 clientID;
	ItemID itemID;
	en::F32 remainingDistance;

	bool Update(en::F32 dtSeconds);
};
sf::Packet& operator<<(sf::Packet& packet, const Bullet& bullet);
sf::Packet& operator>>(sf::Packet& packet, Bullet& bullet);

struct Blood
{
	en::U32 bloodUID;
	en::Vector2f position;
	en::Time remainingTime;
};
sf::Packet& operator<<(sf::Packet& packet, const Blood& blood);
sf::Packet& operator>>(sf::Packet& packet, Blood& blood);
