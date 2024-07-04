#ifndef trRigidbody
#define trRigidbody

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>

class Rigidbody
{
private:
	b2BodyDef bodyDef;
	b2Body *body;
	b2FixtureDef fixDef;
	b2Fixture *fixture;
	b2PolygonShape shape;
	sf::Vector2f size;
	sf::String userData;
public:
	Rigidbody();
	void create(sf::Vector2f pos, sf::Vector2f size, float friction, float density, float restitution, float angle = 0, bool fixedAngle = true, bool dynamic = true, int collisionGroup = 0);
	void reset(b2World *world);
	void resize(b2World *world, sf::Vector2f size);
	sf::Vector2f getSize();
	b2Body *getBody();
	void draw(sf::RenderTarget *target);
	void setPosition(sf::Vector2f pos);
	sf::Vector2f getPosition();
	void setDensity(b2World *world, float density);
	void setAngle(float angle);
	float getAngle();
	void destroy(b2World *world);
	void reloadFixture();
	void setUserData(sf::String data);
	sf::String getUserData();
};

#endif