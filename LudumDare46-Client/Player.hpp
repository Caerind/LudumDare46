#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <Enlivengine/System/PrimitiveTypes.hpp>

#include <Enlivengine/Application/ResourceManager.hpp>
#include <Enlivengine/Graphics/SFMLResources.hpp>

#include <Common.hpp>
#include <string>

struct Player
{
	en::U32 clientID;
	std::string nickname;
	Chicken chicken;

	sf::Sprite sprite;
	en::Time walkingTime;
	en::U32 animIndex;

	void UpdateSprite()
	{
		sprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>(GetItemTextureName(chicken.itemID)).Get());
		sprite.setTextureRect(sf::IntRect(animIndex * 64, 0, 64, 64));
		sprite.setPosition(en::toSF(chicken.position));
		sprite.setRotation(chicken.rotation + 90.0f);
	}
};