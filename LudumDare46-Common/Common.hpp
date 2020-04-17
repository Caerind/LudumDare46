#pragma once

#include <Enlivengine/System/PrimitiveTypes.hpp>

#define DefaultServerAddress "92.222.79.62"
#define DefaultServerPort 3457

enum class PacketID : en::U32
{
	Position = 0, // Client -> Server
	UpdatePosition = 1, // Server -> Client
	Count // Can be used for security checks
};
