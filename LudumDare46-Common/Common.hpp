#pragma once

#include <Enlivengine/System/PrimitiveTypes.hpp>

#define DefaultServerAddress "92.222.79.62"
#define DefaultServerPort 3457

#define DefaultTimeout en::seconds(1.0f)
#define DefaultStepInterval en::seconds(1.0f / 60.f)
#define DefaultTickInterval en::seconds(1.0f / 20.f)
#define DefaultSleepTime sf::milliseconds(5)
#define DefaultMaxPlayers 8

// Server -> Client
enum class ServerPacketID : en::U8
{
	Ping = 0, // "Just want the client to send back a Pong"
	Pong, // "Just sending back a Pong when some client asked for Ping"
	ClientAccept, // "Ok, you can come in"
	ClientReject, // "I don't want you here"
	ClientJoined, // "Someone joined"
	ClientLeft, // "Someone left"
	Stopping, // "The server is stopping"

	// Game specific packets
	PlayerPosition,



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
	PlayerPosition,


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
	Banned,

};
