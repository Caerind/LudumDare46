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
		sprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>("shuriken_chicken").Get());
		sprite.setPosition(en::toSF(chicken.position));
		sprite.setRotation(chicken.rotation);
	}
};

struct Seed
{
	en::U32 seedID;
	en::Vector2f position;
};