#include "hRigidbody.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"

#include <iostream>

Rigidbody::Rigidbody()
{
	bodyDef = b2BodyDef();
	body = nullptr;
	fixDef = b2FixtureDef();
	fixture = nullptr;
	shape = b2PolygonShape();
	size = {0, 0};
}

void Rigidbody::create(sf::Vector2f pos, sf::Vector2f size, float friction, float density, float restitution, float angle, bool fixedAngle, bool dynamic, int cg)
{
	body = nullptr;
	fixture = nullptr;
	bodyDef = b2BodyDef();
	fixDef = b2FixtureDef();
	this->size = size;
	if (pos != sf::Vector2f(-1, -1)) bodyDef.position.Set(pos.x / tr::M2P, pos.y / tr::M2P);
	bodyDef.type = (dynamic ? b2_dynamicBody : b2_staticBody);
	bodyDef.angle = angle * tr::DEGTORAD;
	bodyDef.fixedRotation = fixedAngle;
	shape = b2PolygonShape();
	shape.SetAsBox(this->size.x / 2 / tr::M2P , this->size.y / 2 / tr::M2P);
	fixDef.shape = &shape;
	fixDef.friction = friction;
	fixDef.density = density;
	fixDef.restitution = restitution;
	fixDef.filter.groupIndex = cg;
	userData = "";
	bodyDef.userData.pointer = (uintptr_t)&userData;
}

void Rigidbody::reset(b2World *world)
{
	if (!world) { return; }
	destroy(world);
	body = world->CreateBody(&bodyDef);
	fixDef.shape = &shape;
	fixture = body->CreateFixture(&fixDef);
}

b2Body *Rigidbody::getBody() { return body; }

void Rigidbody::draw(sf::RenderTarget *target)
{
	if (!body) { return; }
	sf::RectangleShape rs({size.x, size.y});
	rs.setRotation(body->GetAngle() * tr::RADTODEG);
	rs.setOrigin(size.x / 2, size.y / 2);
	rs.setPosition(body->GetPosition().x * tr::M2P, body->GetPosition().y * tr::M2P);
	rs.setFillColor({0, 0, 0, 0});
	rs.setOutlineColor(sf::Color::Green);
	rs.setOutlineThickness(-2);
	target->draw(rs);
	sf::CircleShape center(2);
	center.setOrigin(2, 2);
	center.setFillColor(sf::Color::Green);
	center.setPosition(getPosition().x, getPosition().y);
	target->draw(center);
}

void Rigidbody::resize(b2World *world, sf::Vector2f size)
{
	this->size = size;
	shape.SetAsBox(this->size.x / 2 / tr::M2P, this->size.y / 2 / tr::M2P);
	fixDef.shape = &shape;
	reloadFixture();
}

sf::Vector2f Rigidbody::getSize() { return size; }

void Rigidbody::setPosition(sf::Vector2f pos)
{
	bodyDef.position.Set(pos.x / tr::M2P, pos.y / tr::M2P);
	if (body != nullptr) body->SetTransform({pos.x / tr::M2P, pos.y / tr::M2P}, body->GetAngle());
}

sf::Vector2f Rigidbody::getPosition()
{
	if (body == nullptr) { return {0, 0}; }
	return {
		body->GetPosition().x * tr::M2P,
		body->GetPosition().y * tr::M2P
	};
}

void Rigidbody::setDensity(b2World *world, float density)
{
	fixDef.density = density;
	reset(world);
}

void Rigidbody::setAngle(float angle)
{
	body->SetTransform(body->GetPosition(), angle * tr::DEGTORAD);
}

float Rigidbody::getAngle()
{
	return body->GetAngle() * tr::RADTODEG;
}

void Rigidbody::destroy(b2World *world)
{
	if (body != nullptr)
	{
		if (fixture != nullptr) body->DestroyFixture(fixture);
		world->DestroyBody(body);
	}
	body = nullptr;
	fixture = nullptr;
}

void Rigidbody::reloadFixture()
{
	if (!body) { return; }
	if (fixture != nullptr) { body->DestroyFixture(fixture); }
	fixDef.shape = &shape;
	fixture = body->CreateFixture(&fixDef);
	if (!userData.isEmpty()) { body->GetUserData().pointer = (uintptr_t)&userData; }
}

void Rigidbody::setUserData(sf::String data)
{
	userData = data;
	bodyDef.userData.pointer = (uintptr_t)&userData;
	if (body) body->GetUserData().pointer = (uintptr_t)&userData;
}

sf::String Rigidbody::getUserData()
{
	return userData;
}