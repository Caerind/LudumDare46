#include "GameMap.hpp"

#include <Enlivengine/Application/ResourceManager.hpp>

GameMap::GameMap()
{
}

bool GameMap::load()
{
	mMap = en::ResourceManager::GetInstance().Get<en::tmx::Map>("map");

	return mMap.IsValid();
}

void GameMap::render(sf::RenderTarget& target)
{
	if (mMap.IsValid())
	{
		mMap.Get().Render(target);
	}
}