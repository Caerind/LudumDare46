#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <Enlivengine/System/PrimitiveTypes.hpp>

#include <Common.hpp>

struct Player
{
	sf::RectangleShape shape;
	en::U32 clientID;
};