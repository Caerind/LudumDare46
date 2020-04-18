#include "Common.hpp"

sf::Packet& operator <<(sf::Packet& packet, const Chicken& chicken)
{
	packet << chicken.position.x;
	packet << chicken.position.y;
	packet << chicken.rotation;
	packet << chicken.itemID;
	packet << chicken.life;
	packet << chicken.speed;
	packet << chicken.attack;
	return packet;
}

sf::Packet& operator >>(sf::Packet& packet, Chicken& chicken)
{
	packet >> chicken.position.x;
	packet >> chicken.position.y;
	packet >> chicken.rotation;
	packet >> chicken.itemID;
	packet >> chicken.life;
	packet >> chicken.speed;
	packet >> chicken.attack;
	return packet;
}
