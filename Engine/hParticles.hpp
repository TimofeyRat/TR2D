#ifndef trParticles
#define trParticles

#include "hAnimation.hpp"
#include "hRigidbody.hpp"

#include <pugixml.hpp>
#include <vector>

struct Particle
{
	Rigidbody rb;
	FrameAnimator fa;
	sf::ConvexShape shape;
	Particle();
};

struct ParticleTemplate
{
	sf::String name;
	sf::String clrType; sf::Color minClr, maxClr;
	FrameAnimator fa;
	std::vector<sf::Vector2f> points;
	sf::String texType;
	Rigidbody rb;
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
public:
	static std::vector<ParticleTemplate> templates;
};

#endif