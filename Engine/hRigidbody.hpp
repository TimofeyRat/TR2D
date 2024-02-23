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
	b2Vec2 size;
public:
	Rigidbody();
	void create(b2Vec2 pos, b2Vec2 size, float friction, float density, float restitution, float angle = 0, bool fixedAngle = true, bool dynamic = true, int collisionGroup = 0);
	void reset();
	void initBody();
	void resize(b2Vec2 size);
	b2Vec2 getSize();
	b2Body *getBody();
	void draw(sf::RenderTarget *target);
	void setPosition(b2Vec2 pos);
	b2Vec2 getPosition();
	void setDensity(float density);
	void setAngle(float angle);
	float getAngle();
	void destroy();
};

#endif