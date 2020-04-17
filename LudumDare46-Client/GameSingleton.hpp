#pragma once

#include <Enlivengine/System/PrimitiveTypes.hpp>
#include <Enlivengine/System/Config.hpp>

#include <Enlivengine/Application/Application.hpp>
#include <Enlivengine/Graphics/SFMLResources.hpp>

#include <entt/entt.hpp>

#include "ClientSocket.hpp"
#include "GameMap.hpp"

class GameSingleton
{
	public:
		static en::Application* mApplication;
		static ClientSocket mClient;
		static en::Vector2f mLocalPosition;
		static bool mInvalidLocalPosition;
		static GameMap mMap;
		static en::View mMenuView;
		static en::View mView;
};
