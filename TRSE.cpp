#include <pugixml.hpp>

#include <vector>
#include <iostream>

#include <SFML/Graphics.hpp>
#include <math.h>
#include <sstream>

const float DEGTORAD = 0.017453f, RADTODEG=57.29577f;

sf::Text FPS, input;
sf::RenderWindow window;
float deltaTime;

std::vector<sf::String> splitStr(sf::String line, sf::String sep)
{
	std::vector<sf::String> parts;
    std::wstring wideInput = line.toWideString();
    std::wstring wideSeparator = sep.toWideString();
    std::wstringstream ss(wideInput);
    sf::String part;
    bool inQuotes = false;

    while (ss.good()) {
        std::wstring substr;
        std::getline(ss, substr, wideSeparator[0]);
        part = substr;

        if (!inQuotes) {
            parts.push_back(part);
            // Check if the part starts with a quote
            if (part.getSize() > 0 && part[0] == L'\"') {
                inQuotes = true;
            }
        }
        else {
            // Add the part to the last element in the vector rather than creating a new element
            parts.back() += sep + part;
            // Check if the part ends with a quote
            if (part.getSize() > 0 && part[part.getSize() - 1] == L'\"') {
                inQuotes = false;
            }
        }
    }
	for (int i = 0; i < parts.size(); i++)
	{
		if (parts[i].isEmpty()) { parts.erase(parts.begin() + i--); continue; }
		if (parts[i].toAnsiString()[0] == '"')
		{
			parts[i].erase(0);
			parts[i].erase(parts[i].find("\""));
		}
	}
    return parts;
}

class Skeleton
{
public:
	struct Bone
	{
		int id, root, layer;
		float length, angle;
		sf::Vector2f pos;
		sf::Texture tex;
		sf::Sprite spr;
		std::string texPath;
		sf::Vector3f angle_origin;
		Bone();
		Bone(int ID,
			int Root,
			float Length,
			float Angle,
			std::string Tex,
			sf::IntRect tRect = {0, 0, 0, 0},
			sf::Vector3f ao = {0, 0, 0},
			int Layer = 0);
		sf::Vector2f getEnd(float scale);
	};
	struct Animation
	{
		struct Frame
		{
			float r;
			sf::IntRect tRect;
			int layer, root;
			float timestamp;
			sf::Vector2f origin;
			void init();
			Frame();
			Frame(pugi::xml_node node);
			void parse(pugi::xml_node node);
		};
		struct Changer
		{
			int boneID;
			std::vector<Frame> frames;
			Changer();
			Changer(pugi::xml_node node);
			void parse(pugi::xml_node node);
			Frame *getFrame(float timestamp);
			Frame *getNextFrame(float timestamp);
		};
		std::vector<Changer> changes;
		sf::String name;
		float currentFrame, duration;
		bool repeat;
		void init();
		Animation();
		Animation(pugi::xml_node node);
		void parse(pugi::xml_node node);
	};
	std::vector<Bone> bones;
	std::vector<Animation> anims;
	int currentAnim;
	sf::Vector2f position;
	sf::Color color;
	float speed, scale;
	Skeleton();
	Skeleton(std::string filename);
	void init();
	void loadFromFile(std::string filename);
	void updateBones();
	void update(float dt);
	void draw(sf::RenderTarget *target);
	void drawBones(sf::RenderTarget *target);
	Bone *getBone(int ID);
	void setPosition(sf::Vector2f xy);
	sf::Vector2f getPosition();
	void setCurrentAnimation(sf::String name);
	Animation *getCurrentAnim();
	void restart();
	void setColor(sf::Color clr);
	sf::Color getColor();
	void setSpeed(float s);
	float getSpeed();
	void setScale(float s);
	float getScale();
	bool hasAnimationEnded();
	sf::FloatRect generateHitbox();
	sf::Glsl::Vec4 getBonePoints(sf::Uint16 id);
	float getBoneAngle(sf::Uint16 id);
	void save(std::string filename)
	{
		if (filename.find(".trskeleton") == std::string::npos)
		{
			filename += ".trskeleton";
		}
		pugi::xml_document doc;
		for (int i = 0; i < bones.size(); i++)
		{
			auto *b = &bones[i];
			auto n = doc.append_child(L"bone");
			n.append_attribute(L"id") = b->id;
			n.append_attribute(L"root") = b->root;
			n.append_attribute(L"length") = b->length;
			n.append_attribute(L"angle") = b->angle;
			n.append_attribute(L"tex") = pugi::as_wide(b->texPath).c_str();
			auto s = b->spr.getScale();
			n.append_attribute(L"scale") = pugi::as_wide(
				(s.x == (int)s.x ? std::to_string((int)s.x) : std::to_string(s.x)) + " " +
				(s.y == (int)s.y ? std::to_string((int)s.y) : std::to_string(s.y))
			).c_str();
			n.append_attribute(L"offset") = b->angle_origin.x;
			sf::Vector2f o = {b->angle_origin.y, b->angle_origin.z};
			n.append_attribute(L"origin") = pugi::as_wide(
				(o.x == (int)o.x ? std::to_string((int)o.x) : std::to_string(o.x)) + " " +
				(o.y == (int)o.y ? std::to_string((int)o.y) : std::to_string(o.y))
			).c_str();
			n.append_attribute(L"layer") = b->layer;
			auto r = b->spr.getTextureRect();
			n.text() = pugi::as_wide(
				std::to_string(r.left) + " " + std::to_string(r.top) + " " +
				std::to_string(r.width) + " " + std::to_string(r.height)
			).c_str();
		}
		for (int i = 0; i < anims.size(); i++)
		{
			auto *a = &anims[i];
			auto anim = doc.append_child(L"animation");
			anim.append_attribute(L"name") = pugi::as_wide(a->name).c_str();
			anim.append_attribute(L"duration") = pugi::as_wide(std::to_string(a->duration)).c_str();
			anim.append_attribute(L"repeat") = a->repeat;
			for (int j = 0; j < a->changes.size(); j++)
			{
				auto *c = &a->changes[j];
				auto change = anim.append_child(L"change");
				change.append_attribute(L"bone") = c->boneID;
				for (int k = 0; k < c->frames.size(); k++)
				{
					auto *f = &c->frames[k];
					auto frame = change.append_child(L"frame");
					frame.append_attribute(L"timestamp") = pugi::as_wide(std::to_string(f->timestamp)).c_str();
					auto rotate = frame.append_child(L"rotate");
					if (f->r != 0)
					{
						rotate.append_attribute(L"value") = f->r;
					}
					if (f->tRect != sf::IntRect(-1, -1, -1, -1)) frame.append_child(L"rect").text() = pugi::as_wide(
						std::to_string(f->tRect.left) + " " +
						std::to_string(f->tRect.top) + " " +
						std::to_string(f->tRect.width) + " " +
						std::to_string(f->tRect.height)
					).c_str();
					if (f->layer != -1) frame.append_child(L"layer").text() = f->layer;
					sf::Vector2f o = f->origin;
					if (o != sf::Vector2f(-1, -1)) frame.append_child(L"origin").text() = pugi::as_wide(
						(o.x == (int)o.x ? std::to_string((int)o.x) : std::to_string(o.x)) + " " +
						(o.y == (int)o.y ? std::to_string((int)o.y) : std::to_string(o.y))
					).c_str();
					if (f->root != -1) frame.append_child(L"root").text() = f->root;
				}
			}
		}
		doc.save_file(pugi::as_wide(filename).c_str());
	}
	Animation::Changer *getChanger(int id)
	{
		auto *a = getCurrentAnim();
		for (int i = 0; i < a->changes.size(); i++)
		{
			if (a->changes[i].boneID == id) { return &a->changes[i]; }
		}
		return nullptr;
	}
};

Skeleton::Bone::Bone()
{
	id = root = layer = length = angle = 0;
	pos = {0, 0};
	tex = sf::Texture();
	spr = sf::Sprite();
	angle_origin = {0, 0, 0};
	texPath = "";
}

Skeleton::Bone::Bone(int ID,
			int Root,
			float Length,
			float Angle,
			std::string Tex,
			sf::IntRect tRect,
			sf::Vector3f ao,
			int Layer)
{
	id = ID;
	root = Root;
	layer = Layer;
	length = Length;
	angle = Angle;
	pos = {0, 0};
	texPath = Tex;
	tex = sf::Texture();
	if (!Tex.empty())
	{
		tex.loadFromFile(texPath);
		spr = sf::Sprite(tex);
	}
	if (tRect != sf::IntRect(0, 0, 0, 0)) { spr.setTextureRect(tRect); }
	angle_origin = ao;
}

sf::Vector2f Skeleton::Bone::getEnd(float scale)
{
	return pos + sf::Vector2f(
		cos((angle + 90) * DEGTORAD),
		sin((angle + 90) * DEGTORAD)
	) * length * scale;
}

void Skeleton::Animation::Frame::init()
{
	r = -1;
	tRect = {-1, -1, -1, -1};
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
	for (auto part : node.children())
	{
		auto name = sf::String(part.name());
		if (name == "rotate")
		{
			r = part.attribute(L"value").as_float();
		}
		else if (name == "rect")
		{
			auto rect = splitStr(part.text().as_string(), " ");
			tRect = {
				std::stof(rect[0].toAnsiString()), std::stof(rect[1].toAnsiString()),
				std::stof(rect[2].toAnsiString()), std::stof(rect[3].toAnsiString())
			};
		}
		else if (name == "layer") { layer = part.text().as_int(); }
		else if (name == "origin")
		{
			auto o = splitStr(part.text().as_string(), " ");
			origin = {
				std::stof(o[0].toAnsiString()),
				std::stof(o[1].toAnsiString())
			};
		}
		else if (name == "root") { root = part.text().as_int(); }
	}
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
	file.load_file(pugi::as_wide(filename).c_str());
	for (auto node : file.children())
	{
		auto name = sf::String(node.name());
		if (name == "bone")
		{
			auto r = splitStr(node.text().get(), " ");
			if (!r.size()) { r.resize(4, "0"); }
			auto o = splitStr(node.attribute(L"origin").as_string(), " ");
			if (!o.size()) { o.resize(2, "0"); }
			auto b = Bone(
				node.attribute(L"id").as_int(),
				node.attribute(L"root").as_int(),
				node.attribute(L"length").as_float(),
				node.attribute(L"angle").as_float(),
				pugi::as_utf8(node.attribute(L"tex").as_string()),
				{
					std::stof(r[0].toAnsiString()), std::stof(r[1].toAnsiString()),
					std::stof(r[2].toAnsiString()), std::stof(r[3].toAnsiString())
				},
				{
					node.attribute(L"offset").as_float(),
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
	}
}

void Skeleton::updateBones()
{
	for (int i = 0; i < bones.size(); i++)
	{
		if (bones[i].root != bones[i].id)
		{
			auto *r = getBone(bones[i].root);
			if (r == nullptr) bones[i].pos = position;
			else bones[i].pos = getBone(bones[i].root)->getEnd(scale);
		}
		else bones[i].pos = position;
	}
}

void Skeleton::update(float dt)
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
		if (cur->tRect != sf::IntRect(-1, -1, -1, -1)) { b->spr.setTextureRect(cur->tRect); }
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
	if (dt > 0) anim->currentFrame += deltaTime * speed;
}

void Skeleton::draw(sf::RenderTarget *target)
{
	for (int l = 0; l < 3; l++)
	{
		for (int i = 0; i < bones.size(); i++)
		{
			auto *b = &bones[i];
			if (b->layer != l) { continue; }
			if (b->tex.getSize().x != 0) b->spr.setTexture(b->tex);
			b->spr.setPosition(b->pos);
			b->spr.setRotation(b->angle + b->angle_origin.x);
			b->spr.setOrigin(b->angle_origin.y, b->angle_origin.z);
			// b->spr.setColor(color);
			b->spr.setScale(scale, scale);
			target->draw(b->spr);
		}
	}
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
	for (int i = 0; i < bones.size(); i++)
	{
		if (bones[i].id == ID) { return &bones[i]; }
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

Skeleton s("eutihey.trskeleton");

Skeleton::Animation::Frame *cf;
Skeleton::Animation::Changer *cc;
Skeleton::Bone *cb;

//0 - anim, 1 - frame
bool info = false;

void execute(std::string command = "")
{
	auto cmd = command.empty() ? input.getString().toAnsiString() : command;
	cmd = cmd.substr(1, cmd.length() - 2);
	auto args = splitStr(cmd, " ");
	if (args[0] == "t") { s.getCurrentAnim()->currentFrame = std::stof(args[1].toAnsiString()); }
	else if (args[0] == "f")
	{
		if (cb == nullptr) { return; }
		if (cc == nullptr)
		{
			s.getCurrentAnim()->changes.push_back({});
			cc = &s.getCurrentAnim()->changes[s.getCurrentAnim()->changes.size() - 1];
			cc->boneID = cb->id;
		}
		if (args[1] == "id")
		{
			cf = &cc->frames[std::stoi(args[2].toAnsiString())];
			// if (cf) s.getCurrentAnim()->currentFrame = cf->timestamp;
			// if (cf->timestamp < 0) s.getCurrentAnim()->currentFrame = 0;
		}
		else if (args[1] == "setup")
		{
			if (cc->frames.size()) if (cc->frames[0].timestamp == 0) { return; }
			cc->frames.push_back({});
			cf = &cc->frames[cc->frames.size() - 1];
			cf->timestamp = 0;
			std::sort(cc->frames.begin(), cc->frames.end(), [](Skeleton::Animation::Frame a, Skeleton::Animation::Frame b)
			{
				return a.timestamp < b.timestamp;
			});
			for (int i = 0; i < cc->frames.size(); i++)
			{
				if (cc->frames[i].timestamp == 0) { cf = &cc->frames[i]; break; }
			}
		}
		else if (args[1] == "new")
		{
			auto ts = s.getCurrentAnim()->currentFrame;
			for (int i = 0; i < cc->frames.size(); i++) { if (cc->frames[i].timestamp == ts) { return; } }
			cc->frames.push_back({});
			cf = &cc->frames[cc->frames.size() - 1];
			cf->timestamp = ts;
			std::sort(cc->frames.begin(), cc->frames.end(), [](Skeleton::Animation::Frame a, Skeleton::Animation::Frame b)
			{
				return a.timestamp < b.timestamp;
			});
			for (int i = 0; i < cc->frames.size(); i++)
			{
				if (cc->frames[i].timestamp == ts) { cf = &cc->frames[i]; break; }
			}
		}
		else if (args[1] == "del")
		{
			if (cf == nullptr) { return; }
			for (int i = 0; i < cc->frames.size(); i++)
			{
				if (cc->frames[i].timestamp == cf->timestamp)
				{
					cc->frames.erase(cc->frames.begin() + i);
					cf = nullptr;
					return;
				}
			}
		}
		else if (args[1] == "move")
		{
			auto ts = std::stof(args[2].toAnsiString());
			cf->timestamp = ts;
			std::sort(cc->frames.begin(), cc->frames.end(), [](Skeleton::Animation::Frame a, Skeleton::Animation::Frame b)
			{
				return a.timestamp < b.timestamp;
			});
			for (int i = 0; i < cc->frames.size(); i++)
			{
				if (cc->frames[i].timestamp == ts) { cf = &cc->frames[i]; break; }
			}
		}
		else if (args[1] == "clear") { if (cc) cc->frames.clear(); }
		else if (args[1] == "set")
		{
			if (!cf) { return; }
			if (args[2] == "a") { cf->r = std::stof(args[3].toAnsiString()); }
			else if (args[2] == "l") { cf->layer = std::stof(args[3].toAnsiString()); }
			else if (args[2] == "o")
			{
				cf->origin = {
					std::stof(args[3].toAnsiString()),
					std::stof(args[4].toAnsiString())
				};
			}
			else if (args[2] == "root") { cf->root = std::stof(args[3].toAnsiString()); }
			else if (args[2] == "tr")
			{
				cf->tRect = {
					std::stoi(args[3].toAnsiString()), std::stof(args[4].toAnsiString()),
					std::stoi(args[5].toAnsiString()), std::stoi(args[6].toAnsiString())
				};
			}
		}
	}
	else if (args[0] == "d") { s.getCurrentAnim()->duration = std::stof(args[1].toAnsiString()); }
	else if (args[0] == "r") { s.getCurrentAnim()->repeat = std::stoi(args[1].toAnsiString()); }
	else if (args[0] == "i")
	{
		if (args[1] == "f" && cf) { info = true; }
		else if (args[1] == "a") { info = false; }
	}
	else if (args[0] == "a")
	{
		if (args[1] == "new")
		{
			Skeleton::Animation a;
			a.name = args[2];
			s.anims.push_back(a);
			s.currentAnim = s.anims.size() - 1;
			cb = nullptr;
			cc = nullptr;
			cf = nullptr;
		}
		else if (args[1] == "name")
		{
			s.setCurrentAnimation(args[2]);
			cb = nullptr;
			cc = nullptr;
			cf = nullptr;
		}
		else if (args[1] == "id")
		{
			s.setCurrentAnimation(s.anims[std::stoi(args[2].toAnsiString())].name);
			cb = nullptr;
			cc = nullptr;
			cf = nullptr;
		}
		else if (args[1] == "rename") { s.getCurrentAnim()->name = args[2]; }
		else if (args[1] == "del")
		{
			for (int i = 0; i < s.anims.size(); i++)
			{
				if (s.anims[i].name == s.getCurrentAnim()->name) { s.anims.erase(s.anims.begin() + i); }
			}
		}
		else if (args[1] == "copy")
		{
			s.setCurrentAnimation(args[2]);
			s.anims.push_back(*s.getCurrentAnim());
			s.anims[s.anims.size() - 1].name = args[3];
			s.setCurrentAnimation(args[3]);
			cb = nullptr;
			cc = nullptr;
			cf = nullptr;
		}
	}
	else if (args[0] == "save")
	{
		s.save(args[1]);
	}
	else if (args[0] == "open")
	{
		s.loadFromFile(args[1]);
	}
	else if (args[0] == "b")
	{
		cb = s.getBone(std::stoi(args[1].toAnsiString()));
		cf = nullptr;
		if (cb == nullptr) { return; }
		cc = s.getChanger(cb->id);
	}
}

int main(int argc, char* argv[])
{
	window.create(sf::VideoMode(1120, 630), "TRSkeletons");
	window.setVerticalSyncEnabled(true);

	sf::RenderTexture scr;
	scr.create(window.getSize().x / 4 * 3, window.getSize().y);
	sf::View view({0, 0}, {
		window.getSize().x / 4 * 3,
		window.getSize().y
	});
	sf::Sprite viewport;

	sf::Clock clock;

	sf::Font font; font.loadFromFile("res/global/font.ttf");
	FPS.setFont(font);
	FPS.setFillColor(sf::Color::White);
	FPS.setCharacterSize(20);

	input.setFont(font);
	input.setFillColor(sf::Color::White);
	input.setCharacterSize(24);

	bool play = false, enter = false;

	sf::CircleShape cursor(1);
	cursor.setOrigin(cursor.getRadius(), cursor.getRadius());

	if (!s.anims.size()) { s.anims.push_back({}); }

	s.setScale(1);
	s.setSpeed(1);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) { window.close(); }
			else if (event.type == sf::Event::Resized)
			{
				window.setView(sf::View({0, 0, event.size.width, event.size.height}));
				scr.create(event.size.width / 4 * 3, event.size.height);
				view.setSize(event.size.width / 4 * 3, event.size.height);
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				if (enter && event.key.code == sf::Keyboard::Escape) { enter = false; input.setString(""); }
				else if (!enter && event.key.code == sf::Keyboard::Space) { play = !play; info = false; }
				else if (!enter && event.key.code == sf::Keyboard::Tab) { view.setCenter({0, 0}); }
				else if (!enter && event.key.code == sf::Keyboard::Q) { s.restart(); s.update(0); }
				else if (!enter && event.key.code == sf::Keyboard::E) { s.update(0); }
				else if (!enter && event.key.code == sf::Keyboard::I) { execute(info ? ">i a|" : ">i f|"); } //Info
				else if (!enter && event.key.code == sf::Keyboard::N)
				{
					if (event.key.shift || event.key.control || event.key.alt) execute(">f setup|"); //Setup frame
					else execute(">f new|"); //New frame
				}
				else if (!enter && event.key.code == sf::Keyboard::Delete)
				{
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) { execute(">f del|"); }
				}
				else if (event.key.code == sf::Keyboard::Enter)
				{
					enter = !enter;
					if (!enter) { execute(); }
					input.setString(enter ? ">|" : "");
				}
			}
			else if (event.type == sf::Event::MouseWheelMoved)
			{
				if (viewport.getGlobalBounds().contains((sf::Vector2f)sf::Mouse::getPosition(window)))
				{
					if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::C) && cf != nullptr)
					{
						auto delta = -event.mouseWheel.delta;
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) { delta *= 2; }
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) { delta *= 5; }
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) { delta *= 10; }
						cb->angle += delta;
						cf->r = cb->angle = (int)cb->angle;
					}
					else if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::T))
					{
						auto delta = -(float)event.mouseWheel.delta / 10;
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) { delta *= 10; }
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) { delta /= 10; }
						s.getCurrentAnim()->currentFrame += delta;
						if (s.getCurrentAnim()->currentFrame > s.getCurrentAnim()->duration) s.getCurrentAnim()->currentFrame = 0;
						if (s.getCurrentAnim()->currentFrame < 0) s.getCurrentAnim()->currentFrame = s.getCurrentAnim()->duration;
						s.update(0);
					}
					else if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::X) && cb && cc)
					{
						auto delta = -event.mouseWheel.delta;
						int frameID = 0;
						if (cf) for (int i = 0; i < cc->frames.size(); i++)
						{
							if (cc->frames[i].timestamp == cf->timestamp) { frameID = i + delta; }
						}
						execute(">f id " + std::to_string(std::clamp(frameID, 0, (int)cc->frames.size())) + "|");
						if (cf->timestamp < 0) s.getCurrentAnim()->currentFrame = 0;
					}
					else if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
					{
						execute(">a id " + std::to_string(std::clamp(
							s.currentAnim - event.mouseWheel.delta,
							0, (int)s.anims.size() - 1
						)) + "|");
						s.update(0);
					}
					else { if (event.mouseWheel.delta < 0) view.zoom(0.5); if (event.mouseWheel.delta > 0) view.zoom(2); }
				}
			}
			else if (event.type == sf::Event::TextEntered)
			{
				if (!enter) continue;
				if (event.text.unicode == 8)
				{
					if (input.getString().getSize() > 2)
					{
						auto str = input.getString();
						str.erase(str.getSize() - 2);
						input.setString(str);
					}
				}
				else if (event.text.unicode != 13)
				{
					auto str = input.getString();
					str.insert(str.getSize() - 1, sf::String(event.text.unicode));
					input.setString(str);
				}
			}
		}
		if (!window.hasFocus()) { continue; }
		deltaTime = clock.restart().asSeconds();
		cursor.setPosition(scr.mapPixelToCoords(sf::Mouse::getPosition(window)));
		
		auto *anim = s.getCurrentAnim();
		FPS.setPosition(window.getSize().x / 4 * 3, 0);
		input.setOrigin(0, input.getGlobalBounds().height + 8);
		input.setPosition(window.getSize().x / 4 * 3, window.getSize().y);
		auto fps = std::to_string(anim->currentFrame);
		FPS.setString(std::to_string(int(1.0f / deltaTime)));
		if (!info) FPS.setString(FPS.getString() +
			"\nTime: " + fps +
			"\nAnim: " + anim->name +
			"\nDuration: " + std::to_string(anim->duration) +
			"\nRepeat: " + std::to_string(anim->repeat));
		else if (info && cf) FPS.setString(FPS.getString() +
			"\nTime: " + std::to_string(cf->timestamp) +
			"\nLayer: " + std::to_string(cf->layer) +
			"\nOrigin:\n" + std::to_string(cf->origin.x) + "\n" + std::to_string(cf->origin.y) + 
			"\nRoot: " + std::to_string(cf->root) +
			"\nAngle: " + std::to_string(cf->r) +
			"\nRect: " +
				std::to_string(cf->tRect.left) + " " +
				std::to_string(cf->tRect.top) + " " +
				std::to_string(cf->tRect.width) + " " +
				std::to_string(cf->tRect.height));
		else if (info && !cf) { info = false; }

		if (play) { s.update(deltaTime); }
		else { s.updateBones(); }

		float speed = 100;
		if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) { speed *= 2; }
		if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) { speed /= 2; }
		if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { view.move(-speed * deltaTime, 0); }
		if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { view.move(speed * deltaTime, 0); }
		if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { view.move(0, -speed * deltaTime); }
		if (!enter && sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { view.move(0, speed * deltaTime); }
		
		for (int i = 0; i < s.bones.size(); i++)
		{
			auto *b = s.getBone(i);
			b->spr.setColor(sf::Color::White);
			if (b->spr.getGlobalBounds().contains(cursor.getPosition()))
			{
				b->spr.setColor({127, 127, 127});
				if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
				{
					cb = b;
					cc = s.getChanger(cb->id);
					cf = nullptr;
					if (cc == nullptr)
					{
						anim->changes.push_back({});
						cc = &anim->changes[anim->changes.size() - 1];
						cc->boneID = cb->id;
					}
				}
			}
			else { b->spr.setColor(sf::Color::White); }
		}
		
		if (cb)
		{
			cb->spr.setColor({127, 127, 127});
			if (!info) {
			auto rect = cb->spr.getTextureRect();
			FPS.setString(FPS.getString() +
				"\nID: " + std::to_string(cb->id) +
				"\nAngle: " + std::to_string(cb->angle) +
				"\nLayer: " + std::to_string(cb->layer) +
				"\nRoot: " + std::to_string(cb->root) +
				"\nLength: " + std::to_string(cb->length) +
				"\nRect: " +
					std::to_string(rect.left) + " " +
					std::to_string(rect.top) + " " +
					std::to_string(rect.width) + " " +
					std::to_string(rect.height) +
				"\nOrigin: " +
					std::to_string(cb->angle_origin.y) + " " +
					std::to_string(cb->angle_origin.z));
			if (cc)
			{
				FPS.setString(FPS.getString() +
					"\nFrames: " + std::to_string(cc->frames.size()));
				for (int i = 0; i < cc->frames.size(); i++)
				{
					FPS.setString(FPS.getString() + "\n" + (cf ? (cf->timestamp == cc->frames[i].timestamp ? "(Current)" : "") : "") +
						std::to_string(i) + ": " + std::to_string(cc->frames[i].timestamp));
				}
			}
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && !cb->spr.getGlobalBounds().contains(cursor.getPosition())) { cb = nullptr; cc = nullptr; }
		}}

		window.clear();
		scr.clear(sf::Color(127, 127, 127));
		s.draw(&scr);
		s.drawBones(&scr);
		scr.draw(cursor);
		scr.display();
		scr.setView(view);
		viewport.setTexture(scr.getTexture(), true);
		window.draw(viewport);
		window.draw(input);
		window.draw(FPS);
		window.display();
	}
	return 0;
}