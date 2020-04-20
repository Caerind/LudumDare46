#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <Enlivengine/System/PrimitiveTypes.hpp>
#include <Enlivengine/Math/Vector2.hpp>

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
	en::Vector2f lastPos;
	en::F32 lastRotation;
	en::Time animWalk;
	en::U32 animIndex;

	inline en::Vector2f GetPosition() const
	{
		return en::Vector2f::lerp(lastPos, chicken.position, 0.1f);
	}

	inline en::F32 GetRotation() const
	{
		const en::F32 deltaAngle = en::Math::AngleBetween(lastRotation, chicken.rotation);
		return en::Math::AngleMagnitude(lastRotation + deltaAngle * 0.1f);
	}

	void UpdateSprite()
	{
		sprite.setTexture(en::ResourceManager::GetInstance().Get<en::Texture>(GetItemTextureName(chicken.itemID)).Get());
		sprite.setTextureRect(sf::IntRect(0 * 64, 0, 64, 64));
		sprite.setPosition(en::toSF(GetPosition()));
		sprite.setRotation(GetRotation() + 90.0f);
	}
};