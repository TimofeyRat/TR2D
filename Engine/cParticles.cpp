#include "hParticles.hpp"
#include "hGlobal.hpp"
#include "hAssets.hpp"

#include "hWindow.hpp"

#include <iostream>

std::vector<ParticleTemplate> ParticleSystem::templates;

ParticleTemplate::ParticleTemplate()
{
	effects.clear();
	name = texType = "";
	clrType = ""; minClr = maxClr = sf::Color::White;
	fa = FrameAnimator();
	rb = Rigidbody();
	points.clear();
	physics = true;
	collide = true;
}

ParticleTemplate::ParticleTemplate(pugi::xml_node node)
{
	ParticleTemplate();
	parse(node);
}

void ParticleTemplate::parse(pugi::xml_node node)
{
	name = node.attribute(L"name").as_string();
	for (auto part : node.children())
	{
		auto type = sf::String(part.name());
		if (type == "color")
		{
			clrType = part.attribute(L"type").as_string();
			auto min = tr::splitStr(part.attribute(L"min").as_string(), " ");
			minClr = {
				std::stoi(min[0].toAnsiString()), std::stoi(min[1].toAnsiString()),
				std::stoi(min[2].toAnsiString()), std::stoi(min[3].toAnsiString())
			};
			auto max = tr::splitStr(part.attribute(L"max").as_string(), " ");
			maxClr = {
				std::stoi(max[0].toAnsiString()), std::stoi(max[1].toAnsiString()),
				std::stoi(max[2].toAnsiString()), std::stoi(max[3].toAnsiString())
			};
			texType = "color";
		}
		else if (type == "anim")
		{
			fa.loadFromFile(pugi::as_utf8(part.attribute(L"path").as_string()));
			fa.setCurrentAnimation(part.attribute(L"name").as_string());
			texType = "tex";
		}
		else if (type == "shape")
		{
			for (auto vertex : part.children())
			{
				points.push_back({
					vertex.attribute(L"x").as_float(),
					vertex.attribute(L"y").as_float()
				});
			}
		}
		else if (type == "physics")
		{
			if (part.attribute(L"dynamic").as_int() == -1)
			{
				physics = false;
				collide = false;
			}
			else physics = part.attribute(L"dynamic").as_bool();
			if (physics) rb.create({0, 0}, {1, 1},
				part.attribute(L"friction").as_float(),
				part.attribute(L"mass").as_float(),
				part.attribute(L"restitution").as_float(),
				part.attribute(L"angle").as_float(),
				part.attribute(L"fixedAngle").as_bool(),
				true,
				part.attribute(L"collisionGroup").as_int()
			);
		}
		else if (type == "effect")
		{
			Programmable::Variable var;
			var.name = part.attribute(L"name").as_string();
			var.num = part.attribute(L"num").as_float();
			var.str = part.attribute(L"str").as_string();
			bool type = false;
			if (sf::String(part.attribute(L"type").as_string()) == "set") { type = false; }
			if (sf::String(part.attribute(L"type").as_string()) == "add") { type = true; }
			effects.push_back(Inventory::Effect(
				var,
				part.attribute(L"frequency").as_float(),
				part.attribute(L"usages").as_float(),
				type
			));
		}
	}
}

Particle ParticleTemplate::toParticle()
{
	Particle part;
	part.fa = fa;
	if (texType == "color")
	{
		sf::Glsl::Vec4 clr;
		if (clrType == "rand")
		{
			clr.x = tr::randBetween(minClr.r, maxClr.r);
			clr.y = tr::randBetween(minClr.g, maxClr.g);
			clr.z = tr::randBetween(minClr.b, maxClr.b);
			clr.w = tr::randBetween(minClr.a, maxClr.a);
		}
		if (clrType == "randSame")
		{
			float t = tr::randBetween(0, 1);
			clr.x = minClr.r + (maxClr.r - minClr.r) * t;
			clr.y = minClr.g + (maxClr.g - minClr.g) * t;
			clr.z = minClr.b + (maxClr.b - minClr.b) * t;
			clr.w = minClr.a + (maxClr.a - minClr.a) * t;
		}
		if (clrType == "set")
		{
			clr = {
				(int)minClr.r,
				(int)minClr.g,
				(int)minClr.b,
				(int)minClr.a
			};
		}
		part.shape.setFillColor({clr.x, clr.y, clr.z, clr.w});
	}
	if (texType == "tex")
	{
		part.fa.setCurrentAnimation(fa.getCurrentAnim()->name);
		part.shape.setTexture(part.fa.getCurrentAnim()->texture);
	}
	part.colliding = collide;
	part.physics = physics;
	if (physics) part.rb = rb;
	part.shape.setPointCount(points.size());
	part.effects = effects;
	for (int i = 0; i < points.size(); i++)
	{
		part.shape.setPoint(i, points[i]);
	}
	return part;
}

Particle::Particle()
{
	effects.clear();
	rb = Rigidbody();
	fa = FrameAnimator();
	shape = sf::ConvexShape();
	timer = life = 0;
	destroyed = false;
	physics = true;
	colliding = true;
	speed = {0, 0};
}

void Particle::reset(b2World *world)
{
	rb.reset(world);
	rb.getBody()->SetLinearVelocity({speed.x / tr::M2P, speed.y / tr::M2P});
}

void Particle::update()
{
	if (destroyed) return;
	if (life > 0)
	{
		timer += Window::getDeltaTime();
		if (timer >= life) { destroyed = true; return; }
	}
	shape.setOrigin(shape.getLocalBounds().getSize() / 2.0f);
	if (!physics)
	{
		shape.move(speed * Window::getDeltaTime());
		shape.setRotation(atan2f(speed.y, speed.x) * tr::RADTODEG);
	}
	else
	{
		shape.setPosition(rb.getPosition().x, rb.getPosition().y);
		rb.getBody()->SetTransform(rb.getBody()->GetPosition(), atan2(
			rb.getBody()->GetLinearVelocity().y,
			rb.getBody()->GetLinearVelocity().x
		) - 90 * tr::DEGTORAD);
		shape.setRotation(rb.getAngle());
	}
}

void ParticleSystem::init()
{
	templates.clear();

	pugi::xml_document file;
	file.load_string(AssetManager::getText(AssetManager::path + "worlds/particles.trconf").toWideString().c_str());
	for (auto temp : file.children())
	{
		templates.push_back(temp);
	}
}

Particle ParticleSystem::createParticle(b2World *world, sf::String name, sf::Vector2f pos, sf::Vector2f speed)
{
	Particle part;
	for (int i = 0; i < templates.size(); i++)
	{
		if (templates[i].name == name) { part = templates[i].toParticle(); }
	}
	if (part.physics)
	{
		part.rb.reset(world);
		part.rb.resize(world, {
			part.shape.getLocalBounds().width,
			part.shape.getLocalBounds().height
		});
		part.rb.setPosition({pos.x, pos.y});
		part.rb.getBody()->SetLinearVelocity({speed.x / tr::M2P, speed.y / tr::M2P});
	}
	else
	{
		part.speed = speed;
		part.shape.setPosition(pos);
	}
	return part;
}

ParticleTemplate ParticleSystem::getTemplate(sf::String name)
{
	for (int i = 0; i < templates.size(); i++)
	{
		if (templates[i].name == name) { return templates[i]; }
	}
	return ParticleTemplate();
}