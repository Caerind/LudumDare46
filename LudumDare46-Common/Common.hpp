#pragma once

#include <Enlivengine/System/PrimitiveTypes.hpp>

#define DefaultServerAddress (92,222,79,62)
#define DefaultServerPort 3456

enum class PacketID : en::U32
{
	Position, // Client -> Server
	UpdatePosition, // Server -> Client
	Count // Can be used for security checks
};