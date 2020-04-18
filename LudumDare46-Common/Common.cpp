#include "Common.hpp"

sf::Packet& operator <<(sf::Packet& packet, const Chicken& chicken)
{
	packet << chicken.position.x;
	packet << chicken.position.y;
	packet << chicken.rotation;
	packet << static_cast<en::U32>(chicken.itemID);
	packet << chicken.life;
	packet << chicken.speed;
	packet << chicken.attack;
	return packet;
}

sf::Packet& operator >>(sf::Packet& packet, Chicken& chicken)
{
	en::U32 itemIDRaw;
	packet >> chicken.position.x;
	packet >> chicken.position.y;
	packet >> chicken.rotation;
	packet >> itemIDRaw;
	packet >> chicken.life;
	packet >> chicken.speed;
	packet >> chicken.attack;
	chicken.itemID = static_cast<ItemID>(itemIDRaw);
	return packet;
}
