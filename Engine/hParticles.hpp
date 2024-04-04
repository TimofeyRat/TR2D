#ifndef trParticles
#define trParticles

#include "hAnimation.hpp"
#include "hRigidbody.hpp"
#include "hInventory.hpp"

#include <pugixml.hpp>
#include <vector>

struct Particle
{
	std::vector<Inventory::Effect> effects;
	Rigidbody rb;
	FrameAnimator fa;
	sf::ConvexShape shape;
	float timer, life;
	sf::Vector2f speed;
	bool destroyed, physics;
	Particle();
	void reset(b2World *world);
};

struct ParticleTemplate
{
	std::vector<Inventory::Effect> effects;
	sf::String name;
	sf::String clrType; sf::Color minClr, maxClr;
	FrameAnimator fa;
	std::vector<sf::Vector2f> points;
	sf::String texType;
	Rigidbody rb;
	bool physics;
	ParticleTemplate();
	ParticleTemplate(pugi::xml_node node);
	void parse(pugi::xml_node node);
	Particle toParticle();
};

class ParticleSystem
{
public:
	static void init();
	static Particle createParticle(b2World *world, sf::String name, sf::Vector2f pos, sf::Vector2f speed);
	static ParticleTemplate getTemplate(sf::String name);
public:
	static std::vector<ParticleTemplate> templates;
};

#endif