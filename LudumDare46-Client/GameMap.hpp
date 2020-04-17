#pragma once

#include <Enlivengine/Map/Map.hpp>

class GameMap
{
public:
	GameMap();

	void load(en::tmx::MapPtr mapPtr);
	void render(sf::RenderTarget& target);
	
private:
    en::tmx::MapPtr mMap;
};
