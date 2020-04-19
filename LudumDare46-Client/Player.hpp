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

	void UpdateSprite()
	{
		switch (chicken.itemID)
		{
		case ItemID::None:
			sprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("chicken_vanilla").Get());
			break;
		case ItemID::Shuriken:
			sprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("chicken_shuriken").Get());
			break;
		case ItemID::Laser:
			sprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("chicken_laser").Get());
			break;
		default: break;
		}
		sprite.setTextureRect(sf::IntRect(0, 0, 64, 64));
		sprite.setPosition(en::toSF(chicken.position));
		sprite.setRotation(chicken.rotation + 90.0f);
	}
};

struct Seed
{
	en::U32 seedID;
	en::Vector2f position;
	en::U32 clientID;
};

struct Item
{
	en::U32 itemID;
	ItemID item;
	en::Vector2f position;
};

struct Bullet
{
	en::Vector2f position;
	en::F32 rotation;
	ItemID itemID;
	en::F32 remainingDistance;
};