#include "hRigidbody.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"

Rigidbody::Rigidbody()
{
	bodyDef = b2BodyDef();
	body = nullptr;
	fixDef = b2FixtureDef();
	fixture = nullptr;
	shape = b2PolygonShape();
	size = {0, 0};
}

void Rigidbody::create(b2Vec2 pos, b2Vec2 size, float friction, float density, float restitution, float angle, bool fixedAngle, bool dynamic, int cg)
{
	body = nullptr;
	fixture = nullptr;
	this->size = size;
	if (pos != b2Vec2(-1, -1)) bodyDef.position.Set(pos.x / tr::M2P, pos.y / tr::M2P);
	bodyDef.type = (dynamic ? b2_dynamicBody : b2_staticBody);
	bodyDef.angle = angle * tr::DEGTORAD;
	bodyDef.fixedRotation = fixedAngle;
	shape.SetAsBox(this->size.x / 2 / tr::M2P , this->size.y / 2 / tr::M2P);
	fixDef.shape = &shape;
	fixDef.friction = friction;
	fixDef.density = density;
	fixDef.restitution = restitution;
	fixDef.filter.groupIndex = cg;
}

void Rigidbody::reset()
{
	if (body != nullptr)
	{
		if (fixture != nullptr)
		{
			body->DestroyFixture(fixture);
		}
		World::world->DestroyBody(body);
	}
	body = World::world->CreateBody(&bodyDef);
	fixture = body->CreateFixture(&fixDef);
}

void Rigidbody::initBody()
{
	if (body == nullptr) { body = World::world->CreateBody(&bodyDef); }
	if (fixture != nullptr) { body->DestroyFixture(fixture); }
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
	rs.setOutlineColor(sf::Color::Red);
	rs.setOutlineThickness(-2);
	target->draw(rs);
}

void Rigidbody::resize(b2Vec2 size)
{
	this->size = size;
	shape.SetAsBox(this->size.x / 2 / tr::M2P, this->size.y / 2 / tr::M2P);
	fixDef.shape = &shape;
	initBody();
}

b2Vec2 Rigidbody::getSize() { return size; }

void Rigidbody::setPosition(b2Vec2 pos)
{
	bodyDef.position.Set(pos.x / tr::M2P, pos.y / tr::M2P);
	if (body != nullptr) body->SetTransform({pos.x / tr::M2P, pos.y / tr::M2P}, body->GetAngle());
}

b2Vec2 Rigidbody::getPosition()
{
	if (body == nullptr) { return {0, 0}; }
	return {
		body->GetPosition().x * tr::M2P,
		body->GetPosition().y * tr::M2P
	};
}

void Rigidbody::setDensity(float density)
{
	fixDef.density = density;
	initBody();
}

void Rigidbody::setAngle(float angle)
{
	body->SetTransform(body->GetPosition(), angle * tr::DEGTORAD);
}

float Rigidbody::getAngle()
{
	return body->GetAngle() * tr::RADTODEG;
}

void Rigidbody::destroy()
{
	if (body != nullptr)
	{
		if (fixture != nullptr) body->DestroyFixture(fixture);
		World::world->DestroyBody(body);
	}
}