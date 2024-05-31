#ifndef trWearable
#define trWearable

#include "hInventory.hpp"
#include "hParticles.hpp"

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
	sf::String type;
	bool showOnIdle;
	Weapon();
	void draw(sf::RenderTarget *target, Entity *owner);
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
	void draw(sf::RenderTarget *target, Entity *owner);
};

#endif