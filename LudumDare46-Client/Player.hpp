#pragma once

#include <SFML/Graphics/CircleShape.hpp>
#include <Enlivengine/System/PrimitiveTypes.hpp>

#include <Common.hpp>
#include <string>

struct Player
{
	en::U32 clientID;
	std::string nickname;
	Chicken chicken;
};