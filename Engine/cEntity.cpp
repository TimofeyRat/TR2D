#include "hWorld.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hUI.hpp"

#include <iostream>

Entity::Entity()
{
	reset();
}

Entity::Entity(pugi::xml_node node)
{
	reset();
	loadFromNode(node);
}

Entity::Entity(sf::String file)
{
	reset();
	loadFromFile(file);
}

void Entity::reset()
{
	s = Skeleton();
	rb = Rigidbody();
	effects.clear();
	weapon = Weapon();
	bauble = Bauble();
	clear();
}

void Entity::loadFromFile(sf::String path)
{
	pugi::xml_document doc;
	doc.load_string(pugi::as_wide(AssetManager::getText(path.toAnsiString())).c_str());
	loadFromNode(doc.first_child());
}

void Entity::loadFromNode(pugi::xml_node character)
{
	name = character.attribute(L"name").as_string();
	s.loadFromFile(pugi::as_utf8(character.attribute(L"skeleton").as_string()));
	clear();
	setVar("rotation", 1);
	for (auto node : character.children())
	{
		if (sf::String(node.name()) == "physics")
		{
			rb.create({0, 0}, {1, 1},
				node.attribute(L"friction").as_float(),
				node.attribute(L"mass").as_float(),
				node.attribute(L"restitution").as_float(),
				node.attribute(L"angle").as_float(),
				node.attribute(L"fixedAngle").as_bool(),
				node.attribute(L"dynamic").as_bool(),
				node.attribute(L"collisionGroup").as_int()
			);
		}
		else if (sf::String(node.name()) == "variable")
		{
			setVar(node.attribute(L"name").as_string(), node.attribute(L"string").as_string());
			setVar(node.attribute(L"name").as_string(), node.attribute(L"num").as_float());
		}
	}
}

void Entity::update()
{
	if (!isAlive()) { return; }
	setVar("baubleSpeed", 0);
	if (bauble.id != "null")
	{
		if (!bauble.idleOrActive) { for (int i = 0; i < bauble.effects.size(); i++) { effects.push_back(bauble.effects[i]); } }
		if (getVar("usedBauble") && bauble.timer.getElapsedTime().asSeconds() >= bauble.useDelay)
		{
			setVar("usedBauble", 0);
			bauble.timer.restart();
			for (int i = 0; i < bauble.effects.size(); i++) { effects.push_back(bauble.effects[i]); }
		}
	}
	applyEffects();
	float hurtEffect = getVar("hurtEffect");
	if (hurtEffect > 0)
	{
		hurtEffect = tr::lerp(hurtEffect, 0, Window::getDeltaTime() * 5);
		setVar("hurtEffect", hurtEffect);
		s.setColor({255, 255 - hurtEffect, 255 - hurtEffect, 255});
	}
	auto scale = getVar("scale").num;
	updateRB(scale);
	s.setSpeed(getVar("speed")); s.setScale(scale);
	updateAttack();
	if (getVar("attacking") && (s.hasAnimationEnded() || weapon.id == "null")) { setVar("attacking", 0); }
	// updateAnim();
	s.setCurrentAnimation(getVar(getVar("anim")));
	s.updateBones();
	auto rect = s.generateHitbox();
	rb.resize(rb.getBody()->GetWorld(), {!rect.width ? 1 : rect.width, !rect.height ? 1 : rect.height - 2});
	s.setPosition({
		rb.getPosition().x,
		rb.getPosition().y - rect.height / 2 + (s.getPosition().y - rect.top)
	});
	s.update();
	setVar("posX", rb.getPosition().x);
	setVar("posY", rb.getPosition().y);
	setVar("moveX", rb.getBody()->GetLinearVelocity().x);
	setVar("moveY", rb.getBody()->GetLinearVelocity().y);
	setVar("noHurtTimer", getVar("noHurtTimer") + Window::getDeltaTime());
	setVar("damageCD", getVar("damageCooldown").num);
}

void Entity::draw(sf::RenderTarget *target)
{
	if (!isAlive()) { return; }
	s.draw(target);
	if (!getVar("moveX")) bauble.draw(this);
	if (getVar("attacking")) weapon.draw(this);
	if (Window::getVar("debug")) rb.draw(target);
}

// void Entity::updateAnim()
// {
// 	float moveX, moveY, bodyX, bodyY;
// 	if (auto *b = rb.getBody())
// 	{
// 		bodyX = b->GetLinearVelocity().x;
// 		bodyY = b->GetLinearVelocity().y;
// 	}
// 	moveX = getVar("dx");
// 	moveY = getVar("dy");

// 	float dx = 0, dy = 0;
	
// 	if (moveX < 0) dx = -1;
// 	if (moveX > 0) dx = 1;
// 	if (bodyX < 0) { if (moveX >= 0) dx = -0.5; setVar("rotation", -1); }
// 	if (bodyX > 0) { if (moveX <= 0) dx = 0.5; setVar("rotation", 1); }
// 	dy = getVar("onGround") ? 0 : bodyY;

// 	sf::String anim;
// 	if (dx == -1) { anim = "wl"; } //walkLeft
// 	if (dx == -0.5) { anim = "sl"; } //stopLeft
// 	if (dx == 1) { anim = "wr"; } //walkRight
// 	if (dx == 0.5) { anim = "sr"; } //stopRight
// 	if (!dx) { auto r = getVar("rotation").num; if (r < 0) anim = "il"; if (r > 0) anim = "ir"; } //idle
// 	if (dy < 0) { auto r = getVar("rotation").num; if (r < 0) anim = "jl"; if (r > 0) anim = "jr"; } //Jump
// 	if (dy > 0) { auto r = getVar("rotation").num; if (r < 0) anim = "fl"; if (r > 0) anim = "fr"; } //Fall
// 	s.setCurrentAnimation(getVar(anim));
// 	setVar("anim", getVar(anim).str);
// }

void Entity::setPosition(sf::Vector2f pos) { rb.setPosition({pos.x, pos.y}); }
sf::Vector2f Entity::getPosition() { return s.getPosition(); }

sf::FloatRect Entity::getHitbox() { return s.generateHitbox(); }

void Entity::addEffect(Inventory::Effect effect)
{
	effects.push_back(effect);
}

bool Entity::isAlive()
{
	if (!hasVar("HP")) { return true; }
	return getVar("HP").num > 0;
}

Skeleton *Entity::getSkeleton() { return &s; }

void Entity::applyEffects()
{
	for (int i = 0; i < effects.size(); i++)
	{
		auto *e = &effects[i];
		if (e->timer.getElapsedTime().asSeconds() >= e->frequency)
		{
			if (e->setOrAdd)
			{
				setVar(e->var.name, getVar(e->var.name).num + e->var.num);
			}
			else
			{
				setVar(e->var.name, e->var.num);
				setVar(e->var.name, e->var.str);
			}
			if (e->var.name == "HP" && e->var.num < 0)
			{
				setVar("hurtEffect", 255);
			}
			e->timer.restart();
			e->used++;
		}
		if (e->used >= e->usageCount) { effects.erase(effects.begin() + i--); }
	}
}

void Entity::updateRB(float scale)
{
	auto *body = rb.getBody();
	if (body)
	{
		auto triggers = World::getTriggers();
		auto triggersCheck = sf::Vector2f(body->GetPosition().x * tr::M2P, body->GetPosition().y * tr::M2P + s.generateHitbox().height / 2 + 4);
		auto speed = getVar("speed") + getVar("baubleSpeed");
		setVar("onGround", 0);
		for (int i = 0; i < triggers.size(); i++)
		{
			if (triggers[i]->rb.getBody()->GetFixtureList()->TestPoint({triggersCheck.x / tr::M2P, triggersCheck.y / tr::M2P}) &&
				triggers[i]->getVar("name").str == "ground")
			{
				setVar("onGround", 1);
			}
		}
		auto x = body->GetLinearVelocity().x, dx = getVar("dx").num, maxSpeed = getVar("maxSpeed").num * speed * scale;
		body->ApplyForceToCenter({dx * getVar("speedX") * speed * scale, 0}, true);
		if (body->GetLinearVelocity().x > maxSpeed) { body->SetLinearVelocity({maxSpeed, body->GetLinearVelocity().y}); }
		else if (body->GetLinearVelocity().x < -maxSpeed) { body->SetLinearVelocity({-maxSpeed, body->GetLinearVelocity().y}); }
		if (getVar("attacking") && getVar("onGround") && !weapon.meleeOrRange) { body->SetLinearVelocity({0, body->GetLinearVelocity().y}); }
		if (getVar("onGround") && getVar("dy")) body->SetLinearVelocity({body->GetLinearVelocity().x, getVar("dy") * getVar("speedY") * scale});
		setVar("moveX", body->GetLinearVelocity().x);
		setVar("moveY", body->GetLinearVelocity().y);
	}
}

void Entity::updateAttack()
{
	if (weapon.id == "null") setVar("attacking", 0);
	if (getVar("attack") &&
		!UI::containsMouse() &&
		weapon.timer.getElapsedTime().asSeconds() >= weapon.useDelay &&
		weapon.id != "null")
	{
		setVar("attacking", 1);
		weapon.timer.restart();
	}
}

Rigidbody *Entity::getRigidbody()
{
	return &rb;
}