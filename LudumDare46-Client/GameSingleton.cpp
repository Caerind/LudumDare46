#include "GameSingleton.hpp"

en::Application* GameSingleton::mApplication;
ClientSocket GameSingleton::mClient;
en::Vector2f GameSingleton::mLocalPosition;
bool GameSingleton::mInvalidLocalPosition = true;
GameMap GameSingleton::mMap;
en::View GameSingleton::mMenuView;
en::View GameSingleton::mView;
