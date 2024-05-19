#include "hWearable.hpp"
#include "hWorld.hpp"
#include "hWindow.hpp"

Weapon::Weapon()
{
	id = "null";
	fa = FrameAnimator();
	spr = sf::Sprite();
	effects.clear();
	useDelay = 0;
	timer.restart();
	origin = {0, 0};
	rotation = 0;
	scale = 1;
	type = "";
};

void Weapon::draw(Entity *owner)
{
	float rot = owner->getVar("rotation");
	auto boneName = owner->getVar("handBone").num;
	auto bone = owner->getSkeleton()->getBonePoints(boneName);
	spr.setPosition(bone.z, bone.w);
	spr.setRotation(owner->getSkeleton()->getBoneAngle(boneName) + rotation * rot);
	spr.setOrigin(origin);
	spr.setScale(scale * rot, scale);
	if (fa.getCurrentAnim())
	{
		fa.update();
		fa.send(spr, false, false);
	}
	World::getScreen()->draw(spr);
	if (Window::getVar("debug"))
	{
		sf::RectangleShape rect;
		auto r = spr.getGlobalBounds();
		rect.setSize(r.getSize());
		rect.setPosition(r.getPosition());
		rect.setFillColor(sf::Color(0, 0, 0, 0));
		rect.setOutlineColor(sf::Color::Red);
		rect.setOutlineThickness(-2);
		World::getScreen()->draw(rect);
	}
}

Bauble::Bauble()
{
	id = "null";
	bone.clear();
	fa = FrameAnimator();
	spr = sf::Sprite();
	effects.clear();
	useDelay = 0;
	timer.restart();
	origin = {0, 0};
	rotation = 0;
	scale = 1;
	idleOrActive = true;
}

void Bauble::draw(Entity *owner)
{
	float rot = owner->getVar("rotation");
	auto boneName = owner->getVar(bone).num;
	auto bonePos = owner->getSkeleton()->getBonePoints(boneName);
	auto boneRot = owner->getSkeleton()->getBoneAngle(boneName);
	spr.setPosition(bonePos.x, bonePos.y);
	spr.setRotation(boneRot + rotation * rot);
	spr.setOrigin(origin);
	spr.setScale(scale * rot, scale);
	fa.update();
	fa.send(spr, false, false);
	World::getScreen()->draw(spr);
	if (Window::getVar("debug"))
	{
		sf::RectangleShape rect;
		auto r = spr.getGlobalBounds();
		rect.setSize(r.getSize());
		rect.setPosition(r.getPosition());
		rect.setFillColor(sf::Color(0, 0, 0, 0));
		rect.setOutlineColor(sf::Color::Red);
		rect.setOutlineThickness(-2);
		World::getScreen()->draw(rect);
	}
}