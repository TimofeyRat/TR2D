#include "hAnimation.hpp"
#include "hGlobal.hpp"
#include "hAssets.hpp"
#include "hWindow.hpp"

#include <math.h>
#include <pugixml.hpp>

FrameAnimator::Animation::Animation()
{
	frames.clear();
	frames_flip.clear();
	duration = currentFrame = 0;
	scaleX = scaleY = 1;
	repeat = false;
	texture = nullptr;
	name = "";
}

FrameAnimator::FrameAnimator()
{
	anims.clear();
	currentAnimation = 0;
}

void FrameAnimator::loadFromFile(std::string filename)
{
	currentAnimation = 0;
	anims.clear();
	pugi::xml_document file;
	file.load_string(AssetManager::getText(filename).toWideString().c_str());
	for (auto anim : file.children())
	{
		Animation a;
		a.name = anim.attribute(L"name").as_string();
		a.duration = anim.attribute(L"duration").as_float();
		a.repeat = anim.attribute(L"repeat").as_bool();
		auto scale = tr::splitStr(anim.attribute(L"scale").as_string(), " ");
		a.scaleX = std::stof(scale[0].toAnsiString());
		a.scaleY = std::stof(scale[1].toAnsiString());
		for (auto node : anim.children())
		{
			if (sf::String(node.name()) == "texture")
			{
				a.texture = AssetManager::getTexture(sf::String(node.attribute(L"path").as_string()));
				auto size = tr::splitStr(node.attribute(L"size").as_string(), " ");
				int sizeX = std::stoi(size[0].toAnsiString()),
					sizeY = std::stoi(size[1].toAnsiString()),
					offset = node.attribute(L"start").as_int(),
					length = node.attribute(L"count").as_int();	
				auto texCountX = a.texture->getSize().x / sizeX;
				for (int i = offset; i < offset + length; i++)
				{
					auto r = sf::IntRect(
						sf::Vector2i(
							sizeX * (i % texCountX),
							sizeY * (i / texCountX)
						),
						sf::Vector2i(sizeX, sizeY)
					);
					a.frames.push_back(r);
					r.left += r.width;
					r.width *= -1;
					a.frames_flip.push_back(r);
				}
			}
			else if (sf::String(node.name()) == "frame")
			{
				a.texture = AssetManager::getTexture(sf::String(node.attribute(L"path").as_string()));
				auto f = tr::splitStr(node.attribute(L"rect").as_string(), " ");
				sf::IntRect frame = {
					std::stoi(f[0].toAnsiString()), std::stoi(f[1].toAnsiString()),
					std::stoi(f[2].toAnsiString()), std::stoi(f[3].toAnsiString())
				};
				a.frames.push_back(frame);
				frame.left += frame.width; frame.width *= -1;
				a.frames_flip.push_back(frame);
			}
		}
		anims.push_back(a);
	}
}

void FrameAnimator::setCurrentAnimation(sf::String currentAnim)
{
	auto previous = currentAnimation;
	for (int i = 0; i < anims.size(); i++)
	{
		if (anims[i].name == currentAnim) { currentAnimation = i; }
	}
	if (currentAnimation != previous)
	{
		for (int i = 0; i < anims.size(); i++) { anims[i].currentFrame = 0; }
	}
}

void FrameAnimator::setCurrentAnimation(sf::Uint16 currentAnim)
{
	if (currentAnimation != currentAnim)
	currentAnimation = currentAnim;
	for (int i = 0; i < anims.size(); i++)
	{
		anims[i].currentFrame = 0;
	}
}

void FrameAnimator::update()
{
	if (anims.size() == 0) { return; }
	auto *anim = &anims[currentAnimation];
	if (anim->currentFrame >= anim->duration)
	{
		if (anim->repeat)
		{
			anim->currentFrame -= anim->duration;
		}
		else { return; }
	}
	anim->currentFrame += Window::getDeltaTime();
}

void FrameAnimator::send(sf::Sprite &spr, bool flip, bool scale)
{
	if (anims.size() == 0) { return; }
	auto *anim = &anims[currentAnimation];
	spr.setTexture(*anim->texture);
	float currentFrame;
	if (anim->duration == 0) currentFrame = 0;
	else currentFrame = anim->currentFrame / anim->duration;
	if (flip)
	{
		if (currentFrame >= 1) spr.setTextureRect(anim->frames_flip[(int)anim->frames_flip.size() - 1]);
		else spr.setTextureRect(anim->frames_flip[(int)(currentFrame * anim->frames_flip.size())]);
	}
	else
	{
		if (currentFrame >= 1) spr.setTextureRect(anim->frames[(int)anim->frames.size() - 1]);
		else spr.setTextureRect(anim->frames[(int)(currentFrame * anim->frames.size())]);
	}
	if (scale && anim->scaleX >= 0 && anim->scaleY >= 0) spr.setScale(anim->scaleX, anim->scaleY);
}

sf::IntRect FrameAnimator::getCurrentFrame(bool flip)
{
	auto *anim = &anims[currentAnimation];
	float currentFrame;
	if (anim->duration == 0) currentFrame = 0;
	else currentFrame = anim->currentFrame / anim->duration;
	if (flip)
	{
		if (currentFrame >= 1) return anim->frames_flip[(int)anim->frames_flip.size() - 1];
		else  return anim->frames_flip[(int)(currentFrame * anim->frames_flip.size())];
	}
	else
	{
		if (currentFrame >= 1) return anim->frames[(int)anim->frames.size() - 1];
		else return anim->frames[(int)(currentFrame * anim->frames.size())];
	}
	return {0, 0, 0, 0};
}

FrameAnimator::Animation *FrameAnimator::getCurrentAnim()
{
	return &anims[currentAnimation];
}

FrameAnimator::Animation *FrameAnimator::getAnim(sf::String anim)
{
	for (int i = 0; i < anims.size(); i++)
	{
		if (anims[i].name == anim) { return &anims[i]; }
	}
	return nullptr;
}

void FrameAnimator::restart() { for (int i = 0; i  < anims.size(); i++) { anims[i].currentFrame = 0; } }

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//Skeleton
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

Skeleton::Texture::Texture()
{
	tex = nullptr;
	name = "";
	rect = {0, 0, 0, 0};
}

Skeleton::Texture::Texture(sf::String id, sf::String path, sf::IntRect tr)
{
	tex = AssetManager::getTexture(path);
	name = id;
	rect = tr;
}

Skeleton::Bone::Bone()
{
	root = layer = length = angle = 0;
	pos = {0, 0};
	spr = sf::Sprite();
	angle_origin = {0, 0, 0};
}

Skeleton::Bone::Bone(int Root,
			float Length,
			sf::Vector3f ao,
			int Layer)
{
	root = Root;
	layer = Layer;
	length = Length;
	pos = {0, 0};
	angle_origin = ao;
}

sf::Vector2f Skeleton::Bone::getEnd(float scale)
{
	return pos + sf::Vector2f(
		cos((angle + 90) * tr::DEGTORAD),
		sin((angle + 90) * tr::DEGTORAD)
	) * length * scale;
}

void Skeleton::Animation::Frame::init()
{
	r = -1;
	tex = "";
	layer = root = -1;
	timestamp = 0;
	origin = {-1, -1};
}

Skeleton::Animation::Frame::Frame()
{
	init();
}

Skeleton::Animation::Frame::Frame(pugi::xml_node node)
{
	parse(node);
}

void Skeleton::Animation::Frame::parse(pugi::xml_node node)
{
	init();
	timestamp = node.attribute(L"timestamp").as_float();
	r = node.attribute(L"rotate").as_float();
	auto tr = tr::splitStr(node.attribute(L"rect").as_string(L"-1 -1 -1 -1"), " ");
	tex = node.attribute(L"texture").as_string();
	layer = node.attribute(L"layer").as_int(-1);
	root = node.attribute(L"root").as_int(-1);
	auto o = tr::splitStr(node.attribute(L"origin").as_string(L"-1 -1"), " ");
	origin = {std::stof(o[0].toAnsiString()), std::stof(o[1].toAnsiString())};
}

Skeleton::Animation::Changer::Changer()
{
	boneID = 0;
	frames.clear();
}

Skeleton::Animation::Changer::Changer(pugi::xml_node node)
{
	parse(node);
}

void Skeleton::Animation::Changer::parse(pugi::xml_node node)
{
	boneID = node.attribute(L"bone").as_int();
	for (auto part : node.children())
	{
		if (sf::String(part.name()) == "frame")
		{
			auto f = Frame(part);
			frames.push_back(f);
		}
	}
}

Skeleton::Animation::Frame *Skeleton::Animation::Changer::getFrame(float timestamp)
{
	int curFrame = 0;
	for (int i = 0; i < frames.size(); i++)
	{
		if (timestamp > frames[i].timestamp) { curFrame = i; }
		else break;
	}
	return frames.size() ? &frames[curFrame] : nullptr;
}

Skeleton::Animation::Frame *Skeleton::Animation::Changer::getNextFrame(float timestamp)
{
	int curFrame = 0;
	for (int i = 0; i < frames.size(); i++)
	{
		if (timestamp > frames[i].timestamp) { curFrame = i; }
		else break;
	}
	if (curFrame + 1 >= frames.size()) { return &frames[0]; }
	else return frames.size() ? &frames[curFrame + 1] : nullptr;
}

void Skeleton::Animation::init()
{
	changes.clear();
	name = "";
	currentFrame = duration = 0;
	repeat = false;
}

Skeleton::Animation::Animation()
{
	init();
}

Skeleton::Animation::Animation(pugi::xml_node node)
{
	parse(node);
}

void Skeleton::Animation::parse(pugi::xml_node node)
{
	init();
	name = node.attribute(L"name").as_string();
	duration = node.attribute(L"duration").as_float();
	repeat = node.attribute(L"repeat").as_bool();
	for (auto part : node.children())
	{
		auto type = sf::String(part.name());
		if (type == "change") changes.push_back(Changer(part));
	}
}

Skeleton::Skeleton()
{
	init();
}

Skeleton::Skeleton(std::string filename)
{
	loadFromFile(filename);
}

void Skeleton::init()
{
	tex.clear();
	bones.clear();
	anims.clear();
	currentAnim = 0;
	position = {0, 0};
	color = {255, 255, 255, 255};
}

void Skeleton::loadFromFile(std::string filename)
{
	if (filename.find(".trskeleton") == std::string::npos) { filename += ".trskeleton"; }
	init();
	pugi::xml_document file;
	file.load_string(AssetManager::getText(filename).toWideString().c_str());
	for (auto node : file.children())
	{
		auto name = sf::String(node.name());
		if (name == "bone")
		{
			auto r = tr::splitStr(node.text().get(), " ");
			if (!r.size()) { r.resize(4, "0"); }
			auto o = tr::splitStr(node.attribute(L"origin").as_string(), " ");
			if (!o.size()) { o.resize(2, "0"); }
			auto b = Bone(
				node.attribute(L"root").as_int(),
				node.attribute(L"length").as_float(),
				{
					node.attribute(L"angleOffset").as_float(),
					std::stof(o[0].toAnsiString()),
					std::stof(o[1].toAnsiString())
				},
				node.attribute(L"layer").as_int()
			);
			bones.push_back(b);
		}
		else if (name == "animation")
		{
			anims.push_back(Animation(node));
		}
		else if (name == "texture")
		{
			auto tr = tr::splitStr(node.attribute(L"rect").as_string(L"0 0 0 0"), " ");
			tex.push_back(Texture(
				node.attribute(L"name").as_string(),
				node.attribute(L"texture").as_string(),
				{
					std::stof(tr[0].toAnsiString()), std::stof(tr[1].toAnsiString()),
					std::stof(tr[2].toAnsiString()), std::stof(tr[3].toAnsiString())
				}
			));
		}
	}
}

void Skeleton::updateBones()
{
	for (int i = 0; i < bones.size(); i++)
	{
		if (bones[i].root != i)
		{
			auto *r = getBone(bones[i].root);
			if (r == nullptr) bones[i].pos = position;
			else bones[i].pos = getBone(bones[i].root)->getEnd(scale);
		}
		else bones[i].pos = position;
	}
}

void Skeleton::update()
{
	updateBones();
	auto *anim = getCurrentAnim();
	if (!anim) { return; }
	if (anim->currentFrame > anim->duration)
	{
		if (anim->repeat) { anim->currentFrame -= anim->duration; }
		else return;
	}
	for (int c = 0; c < anim->changes.size(); c++)
	{
		auto *change = &anim->changes[c];
		if (!change) { continue; }
		auto *b = getBone(change->boneID);
		if (!b) { continue; }
		auto *cur = change->getFrame(anim->currentFrame);
		auto *next = change->getNextFrame(anim->currentFrame);
		if (!cur) continue;
		if (!cur->tex.isEmpty())
		{
			auto t = getTexture(cur->tex);
			b->spr.setTexture(*t->tex);
			b->spr.setTextureRect(t->rect);
		}
		if (cur->layer != -1) { b->layer = cur->layer; }
		if (cur->origin != sf::Vector2f(-1, -1))
		{
			b->angle_origin.y = cur->origin.x;
			b->angle_origin.z = cur->origin.y;
		}
		if (cur->root != -1) { b->root = cur->root; }
		if (!next) { continue; }
		if (cur != next)
		{
			auto angle = cur->timestamp < next->timestamp ? next->r - cur->r : cur->r - next->r;
			auto t = anim->currentFrame - cur->timestamp;
			auto dt = next->timestamp - cur->timestamp;
			b->angle = cur->r + angle / dt * t;
		}
		else { b->angle = cur->r; }
	}
	anim->currentFrame += Window::getDeltaTime() * speed;
}

void Skeleton::draw(sf::RenderTarget *target, const sf::RenderStates &states)
{
	bool debug = Window::getVar("debug");
	for (int l = 0; l < 3; l++)
	{
		for (int i = 0; i < bones.size(); i++)
		{
			auto *b = &bones[i];
			if (b->layer != l) { continue; }
			b->spr.setPosition(b->pos);
			b->spr.setRotation(b->angle + b->angle_origin.x);
			b->spr.setOrigin(b->angle_origin.y, b->angle_origin.z);
			b->spr.setColor(color);
			b->spr.setScale(scale, scale);
			target->draw(b->spr);
			if (debug)
			{
				auto r = b->spr.getGlobalBounds();
				sf::RectangleShape rect(r.getSize());
				rect.setPosition(r.getPosition());
				rect.setFillColor({0, 0, 0, 0});
				rect.setOutlineColor(sf::Color::Red);
				rect.setOutlineThickness(-2);
				target->draw(rect);
			}
		}
	}
	if (debug) drawBones(target);
}

void Skeleton::drawBones(sf::RenderTarget *target)
{
	sf::VertexArray va;
	va.resize(2);
	va[0].color = sf::Color::Red;
	va[1].color = sf::Color::Blue;
	va.setPrimitiveType(sf::PrimitiveType::Lines);
	for (int i = 0; i < bones.size(); i++)
	{
		va[0].position = bones[i].pos;
		va[1].position = bones[i].getEnd(scale);
		target->draw(va);
	}
}

Skeleton::Bone *Skeleton::getBone(int ID)
{
	if (!bones.size() || ID != tr::clamp(ID, 0, bones.size() - 1)) return nullptr;
	return &bones[ID];
}

Skeleton::Texture *Skeleton::getTexture(sf::String name)
{
	for (int i = 0; i < tex.size(); i++)
	{
		if (tex[i].name == name) { return &tex[i]; }
	}
	return nullptr;
}

void Skeleton::setPosition(sf::Vector2f xy)
{
	position = xy;
}

sf::Vector2f Skeleton::getPosition()
{
	return position;
}

void Skeleton::setCurrentAnimation(sf::String name)
{
	auto anim = currentAnim;
	for (int i = 0; i < anims.size(); i++)
	{
		if (anims[i].name == name) { currentAnim = i; }
	}
	if (currentAnim != anim)
	{
		for (int i = 0; i < anims.size(); i++) { anims[i].currentFrame = 0; }
	}
}

Skeleton::Animation *Skeleton::getCurrentAnim()
{
	return &anims[currentAnim];
}

void Skeleton::restart()
{
	for (int i = 0; i < anims.size(); i++)
	{
		anims[i].currentFrame = 0;
	}
}

void Skeleton::setColor(sf::Color clr)
{
	color = clr;
}

sf::Color Skeleton::getColor()
{
	return color;
}

void Skeleton::setSpeed(float s)
{
	speed = s;
}

float Skeleton::getSpeed()
{
	return speed;
}

void Skeleton::setScale(float s)
{
	scale = s;
}

float Skeleton::getScale()
{
	return scale;
}

bool Skeleton::hasAnimationEnded()
{
	return getCurrentAnim()->currentFrame >= getCurrentAnim()->duration;
}

sf::FloatRect Skeleton::generateHitbox()
{
	float x1, x2, y1, y2;
	x1 = x2 = position.x;
	y1 = y2 = position.y;
	for (int i = 0; i < bones.size(); i++)
	{
		auto r = bones[i].spr.getGlobalBounds();
		if (r.left < x1) { x1 = r.left; }
		if (r.top < y1) { y1 = r.top; }
		if (r.left + r.width > x2) { x2 = r.left + r.width; }
		if (r.top + r.height > y2) { y2 = r.top + r.height;}
	}
	return sf::FloatRect(x1, y1, x2 - x1, y2 - y1);
}

sf::Glsl::Vec4 Skeleton::getBonePoints(sf::Uint16 id)
{
	auto *b = getBone(id);
	return {
		b->pos.x, b->pos.y,
		b->getEnd(scale).x, b->getEnd(scale).y
	};
}

float Skeleton::getBoneAngle(sf::Uint16 id)
{
	return getBone(id)->angle;
}