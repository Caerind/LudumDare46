#include "GameMap.hpp"

GameMap::GameMap()
{
}

void GameMap::load(en::tmx::MapPtr mapPtr)
{
	mMap = mapPtr;
}

void GameMap::render(sf::RenderTarget& target)
{
	if (mMap.IsValid())
	{
		mMap.Get().Render(target);
	}
}