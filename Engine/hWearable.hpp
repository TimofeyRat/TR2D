#ifndef trWearable
#define trWearable

#include "hInventory.hpp"

class Entity;

class Weapon
{
public:
	sf::String id;
	FrameAnimator fa;
	sf::Sprite spr;
	std::vector<Inventory::Effect> effects;
	float useDelay;
	sf::Clock timer;
	sf::Vector2f origin;
	float rotation, scale;
	bool meleeOrRange;
	Weapon();
	void draw(Entity *owner);
};

class Bauble
{
public:
	sf::String id, bone;
	FrameAnimator fa;
	sf::Sprite spr;
	std::vector<Inventory::Effect> effects;
	float useDelay;
	sf::Clock timer;
	sf::Vector2f origin;
	float rotation, scale;
	bool idleOrActive;
	Bauble();
	void draw(Entity *owner);
};

#endif