#pragma once

#include <Enlivengine/Map/Map.hpp>

class GameMap
{
public:
	GameMap();

	bool load();
	void render(sf::RenderTarget& target);
	
private:
    en::tmx::MapPtr mMap;
};
