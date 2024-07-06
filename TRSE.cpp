#include <SFML/Graphics.hpp>
#include <pugixml.hpp>
#include "Engine/hGlobal.hpp"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <math.h>

sf::RenderWindow window;
float deltaTime, currentFrame;
sf::Font font;
int currentPage = 0, currentBone, currentTexture, currentAnimation, enterCommandPage, enterCommandID;
sf::Vector2f mouse;
sf::Text enter;
bool typing, play = false;
int nearest;
sf::String res;

//Pages
#define PageCreate 0
#define PageBone 1
#define PageTexture 2
#define PageAnimation 3
#define PageFrame 4

//Page "Create"
#define OpenFile 0
#define Save 1
#define CreateBone 2
#define CreateTexture 3
#define CreateAnimation 4
#define DeleteBone 5
#define DeleteTexture 6
#define DeleteAnimation 7

//Page "Bone"
#define BoneID 0
#define BoneLength 1
#define BoneLayer 2
#define BoneAngle 3
#define BoneAO 4
#define BoneRoot 5
#define BoneNL 6
#define BoneNR 7
#define BoneNF 8
#define BoneDF 9

//Page "Texture"
#define TextureID 0
#define TextureName 1
#define TexturePath 2
#define TextureRect 3

//Page "Animation"
#define AnimationID 0
#define AnimationName 1
#define AnimationDuration 2
#define AnimationRepeat 3
#define AnimationCopyStart 4
#define AnimationCopyEnd 5

//Page "Frame"
#define FrameID 0
#define FrameTS 1
#define FrameRotate 2
#define FrameTex 3
#define FrameLayer 4
#define FrameRoot 5
#define FrameOrigin 6

sf::String openWindow(char* filter = "TR2D Skeleton (*.trskeleton)\0*.trskeleton\0")
{
	auto path = std::filesystem::current_path();
	OPENFILENAME ofn;
	char filename[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = filename;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = "Open file";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrInitialDir = path.string().c_str();

	if (GetOpenFileName(&ofn) == TRUE)
	{
		sf::String file = ofn.lpstrFile;
		file.erase(0, path.string().length() + 1);
		std::filesystem::current_path(path);
		while (tr::strContains(file, "\\")) file.replace("\\", "/");
		res = file.substring(0, file.find("/") + 1);
		return file;
	}
	return "Fail";
}

sf::String saveWindow()
{
	auto path = std::filesystem::current_path();
	OPENFILENAME ofn;
	char filename[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = filename;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFilter = "TR2D Skeleton (*.trskeleton)\0*.trskeleton\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = "Open file";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrInitialDir = path.string().c_str();

	if (GetSaveFileName(&ofn) == TRUE)
	{
		sf::String file = ofn.lpstrFile;
		file.erase(0, path.string().length() + 1);
		std::filesystem::current_path(path);
		while (tr::strContains(file, "\\")) file.replace("\\", "/");
		return file;
	}
	return "Fail"; 
}

class Skeleton
{
public:
	struct Texture
	{
		sf::Texture tex;
		sf::String name, path;
		sf::IntRect rect;
		Texture()
		{
			tex = sf::Texture();
			name = path = "";
			rect = {0, 0, 0, 0};
		}
		Texture(pugi::xml_node node)
		{
			path = node.attribute(L"texture").as_string();
			if (!path.isEmpty()) tex.loadFromFile(res + path);
			name = node.attribute(L"name").as_string();
			auto tr = tr::splitStr(node.attribute(L"rect").as_string(L"0 0 0 0"), " ");
			rect = {
				std::stoi(tr[0].toAnsiString()), std::stoi(tr[1].toAnsiString()),
				std::stoi(tr[2].toAnsiString()), std::stoi(tr[3].toAnsiString())
			};
		}
	};
	struct Bone
	{
		int root, layer;
		float length, angle;
		sf::Vector2f pos;
		sf::Sprite spr;
		sf::Vector3f angle_origin;
		Bone()
		{
			root = layer = length = angle = 0;
			pos = {0, 0};
			spr = sf::Sprite();
			angle_origin = {0, 0, 0};
		}
		Bone(pugi::xml_node node)
		{
			root = node.attribute(L"root").as_int();
			layer = node.attribute(L"layer").as_int();
			length = node.attribute(L"length").as_float();
			angle = 0;
			pos = {0, 0};
			spr = sf::Sprite();
			angle_origin.x = node.attribute(L"angleOffset").as_float();
		}
		sf::Vector2f getEnd(float scale)
		{
			return pos + tr::getDelta(angle + 90) * length * scale;
		}
	};
	struct Animation
	{
		struct Frame
		{
			float r;
			sf::String tex;
			int layer, root;
			float timestamp;
			sf::Vector2f origin;
			void init()
			{
				r = timestamp = 0;
				tex = "";
				layer = root = -1;
				origin = {-1, -1};
			}
			Frame() { init(); }
			Frame(pugi::xml_node node) { parse(node); }
			void parse(pugi::xml_node node)
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
		};
		struct Changer
		{
			int boneID;
			std::vector<Frame> frames;
			Changer()
			{
				boneID = 0;
				frames.clear();
			}
			Changer(pugi::xml_node node) { parse(node); }
			void parse(pugi::xml_node node)
			{
				boneID = node.attribute(L"bone").as_int();
				for (auto part : node.children())
				{
					if (sf::String(part.name()) == "frame")
					{
						frames.push_back(Frame(part));
					}
				}
			}
			Frame *getFrame(float timestamp)
			{
				if (!frames.size()) return nullptr;
				int f = 0;
				while (frames[f].timestamp <= timestamp) f++;
				return &frames[tr::clamp(f - 1, 0, frames.size() - 1)];
			}
			Frame *getNextFrame(float timestamp, bool repeat)
			{
				if (!frames.size()) return nullptr;
				int f = 0;
				while (frames[f].timestamp <= timestamp) f++;
				return f >= frames.size() ? (repeat ? &frames[0] : &frames[frames.size() - 1]) : &frames[f];
			}
		};
		std::vector<Changer> changes;
		sf::String name;
		float currentFrame, duration;
		bool repeat;
		void init()
		{
			changes.clear();
			name = "";
			currentFrame = duration = 0;
			repeat = false;
		}
		Animation() { init(); }
		Animation(pugi::xml_node node) { parse(node); }
		void parse(pugi::xml_node node)
		{
			init();
			name = node.attribute(L"name").as_string();
			duration = node.attribute(L"duration").as_float();
			repeat = node.attribute(L"repeat").as_bool();
			for (auto part : node.children())
			{
				if (sf::String(part.name()) == "change")
				{
					changes.push_back(Changer(part));
					std::sort(
					changes.begin(),
					changes.end(),
					[](Changer a, Changer b)
					{
						return a.boneID < b.boneID;
					}
				);
				}
			}
		}
		Changer *getChanger(int bone)
		{
			for (int i = 0; i < changes.size(); i++)
			{
				if (changes[i].boneID == bone) return &changes[i];
			}
			return nullptr;
		}
	};
	std::vector<Texture> tex;
	std::vector<Bone> bones;
	std::vector<Animation> anims;
	int currentAnim;
	sf::Vector2f position;
	sf::Color color;
	float speed, scale;
	Skeleton() { init(); }
	Skeleton(std::string filename) { loadFromFile(filename); }
	void init()
	{
		tex.clear();
		bones.clear();
		anims.clear();
		currentAnim = 0;
		position = {0, 0};
		color = sf::Color::White;
		speed = scale = 1;

		deltaTime = currentFrame = currentPage = currentBone = currentTexture = currentAnimation = enterCommandID = enterCommandPage = nearest = typing = play = 0;
	}
	void loadFromFile(std::string filename)
	{
		if (!tr::strContains(filename, ".trskeleton")) { filename += ".trskeleton"; }
		init();
		pugi::xml_document file;
		file.load_file(pugi::as_wide(filename).c_str());
		for (auto node : file.children())
		{
			auto name = sf::String(node.name());
			if (name == "texture")
			{
				tex.push_back(Texture(node));
			}
			else if (name == "bone")
			{
				bones.push_back(Bone(node));
			}
			else if (name == "animation")
			{
				anims.push_back(Animation(node));
			}
		}
	}
	void saveAs(std::string filename)
	{
		if (!tr::strContains(filename, ".trskeleton")) filename += ".trskeleton";
		pugi::xml_document file;
		for (int i = 0; i < bones.size(); i++)
		{
			auto bone = file.append_child(L"bone");
			auto b = &bones[i];
			bone.append_attribute(L"root") = b->root;
			bone.append_attribute(L"layer") = b->layer;
			bone.append_attribute(L"length") = b->length;
			bone.append_attribute(L"angleOffset") = b->angle_origin.x;
		}
		for (int i = 0; i < tex.size(); i++)
		{
			auto texture = file.append_child(L"texture");
			auto t = &tex[i];
			texture.append_attribute(L"name") = t->name.toWideString().c_str();
			texture.append_attribute(L"texture") = t->path.toWideString().c_str();
			if (t->rect != sf::IntRect(0, 0, 0, 0))
			texture.append_attribute(L"rect") = sf::String(
				std::to_string(t->rect.left) + " " + std::to_string(t->rect.top) + " " +
				std::to_string(t->rect.width) + " " + std::to_string(t->rect.height)
			).toWideString().c_str();
		}
		for (int i = 0; i < anims.size(); i++)
		{
			auto anim = file.append_child(L"animation");
			auto a = &anims[i];
			anim.append_attribute(L"name") = a->name.toWideString().c_str();
			anim.append_attribute(L"duration") = a->duration;
			anim.append_attribute(L"repeat") = a->repeat;
			for (int j = 0; j < a->changes.size(); j++)
			{
				auto change = anim.append_child(L"change");
				auto c = &a->changes[j];
				change.append_attribute(L"bone") = c->boneID;
				for (int k = 0; k < c->frames.size(); k++)
				{
					auto frame = change.append_child(L"frame");
					auto f = &c->frames[k];
					frame.append_attribute(L"timestamp") = f->timestamp;
					if (f->r != 0) frame.append_attribute(L"rotate") = f->r;
					if (!f->tex.isEmpty()) frame.append_attribute(L"texture") = f->tex.toWideString().c_str();
					if (f->layer != -1) frame.append_attribute(L"layer") = f->layer;
					if (f->root != -1) frame.append_attribute(L"root") = f->root;
					if (f->origin != sf::Vector2f(-1, -1)) frame.append_attribute(L"origin") = sf::String(
						std::to_string(f->origin.x) + " " +
						std::to_string(f->origin.y)
					).toWideString().c_str();
				}
			}
		}
		file.save_file(pugi::as_wide(filename).c_str());
	}
	void updateBones()
	{
		for (int i = 0; i < bones.size(); i++)
		{
			if (bones[i].root != i)
			{
				auto r = getBone(bones[i].root);
				if (!r) bones[i].pos = position;
				else bones[i].pos = r->getEnd(scale);
			}
			else bones[i].pos = position;
		}
	}
	void update()
	{
		updateBones();
		currentAnim = currentAnimation;
		auto anim = getCurrentAnim();
		if (!anim) return;
		anim->currentFrame = currentFrame;
		if (anim->currentFrame > anim->duration)
		{
			if (anim->repeat) { currentFrame -= anim->duration; }
			else return;
		}
		for (int i = 0; i < anim->changes.size(); i++)
		{
			auto c = &anim->changes[i]; if (!c) continue;
			auto b = getBone(c->boneID); if (!b) continue;
			auto current = c->getFrame(anim->currentFrame); if (!current) continue;
			if (!current->tex.isEmpty())
			{
				auto t = getTexture(current->tex);
				b->spr.setTexture(t->tex);
				b->spr.setTextureRect(t->rect);
			}
			if (current->layer != -1) b->layer = current->layer;
			if (current->origin != sf::Vector2f(-1, -1))
			{
				b->angle_origin.y = current->origin.x;
				b->angle_origin.z = current->origin.y;
			}
			if (current->root != -1) b->root = current->root;
			auto next = c->getNextFrame(anim->currentFrame, anim->repeat); if (!next) continue;
			if (current->timestamp != next->timestamp)
			{
				auto angle = current->timestamp < next->timestamp ? next->r - current->r : current->r - next->r;
				auto t = anim->currentFrame - current->timestamp;
				auto dt = next->timestamp - current->timestamp;
				b->angle = current->r + angle / dt * t;
			}
			else b->angle = current->r;
		}
		if (play) currentFrame += deltaTime * speed;
	}
	void draw(sf::RenderTarget *target, const sf::RenderStates &states = sf::RenderStates::Default)
	{
		for (int l = 0; l < 3; l++)
		{
			for (int i = 0; i < bones.size(); i++)
			{
				auto b = &bones[i];
				if (b->layer != l) continue;
				b->spr.setPosition(b->pos);
				b->spr.setRotation(b->angle + b->angle_origin.x);
				b->spr.setOrigin(b->angle_origin.y, b->angle_origin.z);
				b->spr.setColor(color);
				if (currentPage == PageFrame)
				{
					if (i == currentBone) { b->spr.setColor({255, 255, 255, 255}); }
					else if (i == nearest) { b->spr.setColor({255, 255, 255, 200}); }
					else { b->spr.setColor({255, 255, 255, 127}); }
				}
				b->spr.setScale(scale, scale);
				target->draw(b->spr);
			}
		}
	}
	void drawBones(sf::RenderTarget *target)
	{
		sf::VertexArray va(sf::Lines, 2);
		for (int i = 0; i < bones.size(); i++)
		{
			va[0] = {bones[i].pos, sf::Color::Red};
			va[1] = {bones[i].getEnd(scale), sf::Color::Blue};
			target->draw(va);
		}
	}
	Bone *getBone(int ID)
	{
		if (ID != tr::clamp(ID, 0, bones.size() - 1) || !bones.size()) return nullptr;
		return &bones[ID];
	}
	Texture *getTexture(sf::String name)
	{
		if (!tex.size()) return nullptr;
		for (int i = 0; i < tex.size(); i++)
		{
			if (tex[i].name == name) { return &tex[i]; }
		}
		return nullptr;
	}
	void setPosition(sf::Vector2f xy) { position = xy; }
	sf::Vector2f getPosition() { return position; }
	void setCurrentAnimation(sf::String name)
	{
		auto anim = currentAnim;
		for (int i = 0; i < anims.size(); i++)
		{
			if (anims[i].name == name) { currentAnim = i; }
		}
		if (anim != currentAnim) restart();
	}
	Animation *getCurrentAnim() { return &anims[currentAnim]; }
	void restart()
	{
		for (int i = 0; i < anims.size(); i++)
		{
			anims[i].currentFrame = 0;
		}
	}
	void setColor(sf::Color clr) { color = clr; }
	sf::Color getColor() { return color; }
	void setSpeed(float s) { speed = s; }
	float getSpeed() { return speed; }
	void setScale(float s) { scale = s; }
	float getScale() { return scale; }
	bool hasAnimationEnded() { return getCurrentAnim()->currentFrame >= getCurrentAnim()->duration; }
	sf::FloatRect generateHitbox()
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
	float getBoneAngle(sf::Uint16 id) { return getBone(id)->angle; }
};

Skeleton skeleton;

std::vector<sf::Text> ui;

void reloadUI()
{
	ui.clear();
	if (currentPage == PageCreate) ui.resize(8, sf::Text("", font, 20));
	if (currentPage == PageBone) ui.resize(10, sf::Text("", font, 20));
	if (currentPage == PageTexture) ui.resize(4, sf::Text("", font, 20));
	if (currentPage == PageAnimation) ui.resize(6, sf::Text("", font, 20));
	if (currentPage == PageFrame) ui.resize(7, sf::Text("", font, 20));
}

void updateUI()
{
	if (currentPage == PageCreate)
	{
		ui[0].setString("Open file");
		ui[1].setString("Save file");
		ui[2].setString("Create bone");
		ui[3].setString("Create texture");
		ui[4].setString("Create animation");
		ui[5].setString("Delete bone");
		ui[6].setString("Delete texture");
		ui[7].setString("Delete animation");
	}
	if (currentPage == PageBone)
	{
		auto b = skeleton.getBone(currentBone);
		if (!b) { currentPage = PageCreate; reloadUI(); return; }
		ui[0].setString("Bone ID: " + std::to_string(currentBone));
		ui[1].setString("Length: " + std::to_string(b->length));
		ui[2].setString("Layer: " + std::to_string(b->layer));
		ui[3].setString("Angle: " + std::to_string(b->angle));
		ui[4].setString("Sprite angle offset:\n" + std::to_string(b->angle_origin.x));
		ui[5].setString("Root: " + std::to_string(b->root));
		if (auto a = skeleton.getCurrentAnim())
		if (auto changer = a->getChanger(currentBone))
		if (changer->frames.size())
		{
			ui[6].setString("Nearest frame before:\n" + std::to_string(
				changer->getFrame(currentFrame)->timestamp
			));
			ui[7].setString("Nearest frame after:\n" + std::to_string(
				changer->getNextFrame(currentFrame, a->repeat)->timestamp
			));
		}
		ui[8].setString("New frame");
		ui[9].setString("Delete frame");
	}
	if (currentPage == PageTexture)
	{
		if (!skeleton.tex.size() || currentTexture != tr::clamp(currentTexture, 0, skeleton.tex.size() - 1)) { currentPage = PageCreate; reloadUI(); return; }
		ui[0].setString("Texture ID: " + std::to_string(currentTexture));
		ui[1].setString("Name: " + skeleton.tex[currentTexture].name);
		ui[2].setString("Path:\n" + skeleton.tex[currentTexture].path);
		auto rect = skeleton.tex[currentTexture].rect;
		ui[3].setString("Rect:\n" +
			std::to_string(rect.left) + " " + std::to_string(rect.top) + " " +
			std::to_string(rect.width) + " " + std::to_string(rect.height)
		);
	}
	if (currentPage == PageAnimation)
	{
		if (!skeleton.getCurrentAnim()) { currentPage = PageCreate; reloadUI(); return; }
		ui[0].setString("Animation ID: " + std::to_string(currentAnimation));
		ui[1].setString("Name: " + skeleton.anims[currentAnimation].name);
		ui[2].setString("Duration: " + std::to_string(skeleton.anims[currentAnimation].duration));
		ui[3].setString("Repeat: " + std::to_string(skeleton.anims[currentAnimation].repeat));
		ui[4].setString("Copy start of other animation");
		ui[5].setString("Copy end of other animation");
	}
	if (currentPage == PageFrame)
	{
		if (!skeleton.getCurrentAnim()) { currentPage = PageCreate; reloadUI(); return; }
		auto changer = skeleton.getCurrentAnim()->getChanger(currentBone);
		if (!changer) { currentPage = PageCreate; reloadUI(); return; }
		ui[0].setString("Changing bone: " + std::to_string(currentBone));
		auto frame = changer->getFrame(currentFrame);
		ui[1].setString("Change time: " + std::to_string(
			frame->timestamp
		));
		ui[2].setString("Rotate: " + std::to_string(frame->r));
		ui[3].setString("Texture: " + frame->tex);
		ui[4].setString("Layer: " + std::to_string(frame->layer));
		ui[5].setString("Root: " + std::to_string(frame->root));
		ui[6].setString("Origin:\n" +
			std::to_string(frame->origin.x) + " " +
			std::to_string(frame->origin.y)
		);
	}
}

void execute(int page, int id)
{
	if (page == PageCreate)
	{
		if (id == OpenFile)
		{
			auto path = openWindow();
			if (path != "Fail") skeleton.loadFromFile(path);
		}
		if (id == Save)
		{
			auto path = saveWindow();
			if (path != "Fail") skeleton.saveAs(path);
		}
		if (id == CreateBone)
		{
			skeleton.bones.push_back({});
			currentBone = skeleton.bones.size() - 1;
			currentPage = PageBone;
			reloadUI();
		}
		if (id == CreateTexture)
		{
			skeleton.tex.push_back({});
			currentTexture = skeleton.tex.size() - 1;
			currentPage = PageTexture;
			reloadUI();
		}
		if (id == CreateAnimation)
		{
			skeleton.anims.push_back({});
			currentAnimation = skeleton.anims.size() - 1;
			currentPage = PageAnimation;
			reloadUI();
		}
		if (id == DeleteBone) { typing = true; enter.setString("Delete bone|"); }
		if (id == DeleteTexture) { typing = true; enter.setString("Delete texture|"); }
		if (id == DeleteAnimation) { typing = true; enter.setString("Delete animation|"); }
	}
	if (page == PageBone)
	{
		if (id == BoneID) { typing = true; enter.setString("Select bone|"); }
		if (id == BoneLength) { typing = true; enter.setString("New length|"); }
		if (id == BoneLayer) { typing = true; enter.setString("New layer|"); }
		if (id == BoneAngle) { typing = true; enter.setString("New angle|"); }
		if (id == BoneAO) { typing = true; enter.setString("Sprite angle|"); }
		if (id == BoneRoot) { typing = true; enter.setString("New root|"); }
		if (id == BoneNL)
		{
			if (auto anim = skeleton.getCurrentAnim())
			if (auto changer = anim->getChanger(currentBone))
			if (auto frame = changer->getFrame(currentFrame)) currentFrame = frame->timestamp;
			bool p = play;
			play = false;
			skeleton.update();
			play = p;
		}
		if (id == BoneNR)
		{
			currentFrame = skeleton.getCurrentAnim()->
				getChanger(currentBone)->
				getFrame(currentFrame)->timestamp;
			bool p = play;
			play = false;
			skeleton.update();
			play = p;
		}
		if (id == BoneNF)
		{
			auto changer = skeleton.getCurrentAnim()->getChanger(currentBone);
			if (!changer)
			{
				Skeleton::Animation::Changer c; c.boneID = currentBone;
				skeleton.getCurrentAnim()->changes.push_back(c);
				std::sort(
					skeleton.getCurrentAnim()->changes.begin(),
					skeleton.getCurrentAnim()->changes.end(),
					[](Skeleton::Animation::Changer a, Skeleton::Animation::Changer b)
					{
						return a.boneID < b.boneID;
					}
				);
				changer = skeleton.getCurrentAnim()->getChanger(currentBone);
			}
			Skeleton::Animation::Frame f;
			f.timestamp = currentFrame;
			changer->frames.push_back(f);
			std::sort(
				changer->frames.begin(),
				changer->frames.end(),
				[](Skeleton::Animation::Frame a, Skeleton::Animation::Frame b)
				{
					return a.timestamp < b.timestamp;
				}
			);
			currentPage = PageFrame;
		}
		if (id == BoneDF) { typing = true; enter.setString("Timestamp of frame|"); }
	}
	if (page == PageTexture)
	{
		if (id == TextureID) { typing = true; enter.setString("Select texture|"); }
		if (id == TextureName) { typing = true; enter.setString("New name|"); }
		if (id == TexturePath)
		{
			auto path = openWindow("Texture (*.png)\0*.png\0");
			if (path != "Fail") skeleton.tex[currentTexture].path = path;
		}
		if (id == TextureRect) { typing = true; enter.setString("New rect|"); }
	}
	if (page == PageAnimation)
	{
		if (id == AnimationID) { typing = true; enter.setString("Select animation|"); }
		if (id == AnimationName) { typing = true; enter.setString("New name|"); }
		if (id == AnimationDuration) { typing = true; enter.setString("New duration|"); }
		if (id == AnimationRepeat) { typing = true; enter.setString("Repeat(0/1)|"); }
		if (id == AnimationCopyStart) { typing = true; enter.setString("Animation name|"); }
		if (id == AnimationCopyEnd) { typing = true; enter.setString("Animation name|"); }
	}
	if (page == PageFrame)
	{
		if (id == FrameID) { typing = true; enter.setString("ID of bone changer|"); }
		if (id == FrameTS) { typing = true; enter.setString("Time for bone changer|"); }
		if (id == FrameRotate) { typing = true; enter.setString("Bone angle|"); }
		if (id == FrameTex) { typing = true; enter.setString("Bone texture|"); }
		if (id == FrameLayer) { typing = true; enter.setString("Bone layer|"); }
		if (id == FrameRoot) { typing = true; enter.setString("Bone root|"); }
		if (id == FrameOrigin) { typing = true; enter.setString("Bone origin|"); }
	}
	if (typing) { enterCommandPage = page; enterCommandID = id; }
}

void executeEnter()
{
	auto input = enter.getString();
	input = input.substring(input.find("|") + 1);
	if (enterCommandPage == PageCreate)
	{
		if (enterCommandID == DeleteBone)
		{
			skeleton.bones.erase(skeleton.bones.begin() + std::stoi(input.toAnsiString()));
			currentBone = 0;
		}
		if (enterCommandID == DeleteTexture)
		{
			for (int i = 0; i < skeleton.tex.size(); i++)
			{
				if (skeleton.tex[i].name == input)
				{
					skeleton.tex.erase(skeleton.tex.begin() + i);
				}
			}
		}
		if (enterCommandID == DeleteAnimation)
		{
			for (int i = 0; i < skeleton.anims.size(); i++)
			{
				if (skeleton.anims[i].name == input)
				{
					skeleton.anims.erase(skeleton.anims.begin() + i);
				}
			}
		}
	}
	if (enterCommandPage == PageBone)
	{
		auto b = skeleton.getBone(currentBone);
		if (enterCommandID == BoneID) { currentBone = tr::clamp(std::stoi(input.toAnsiString()), 0, skeleton.bones.size() - 1); }
		if (enterCommandID == BoneLength) { b->length = std::stof(input.toAnsiString()); }
		if (enterCommandID == BoneLayer) { b->layer = std::stoi(input.toAnsiString()); }
		if (enterCommandID == BoneAngle) { b->angle = std::stof(input.toAnsiString()); }
		if (enterCommandID == BoneAO) { b->angle_origin.x = std::stof(input.toAnsiString()); }
		if (enterCommandID == BoneRoot) { b->root = std::stoi(input.toAnsiString()); }
		if (enterCommandID == BoneDF)
		{
			if (auto a = skeleton.getCurrentAnim())
			if (auto c = a->getChanger(currentBone))
			{
				for (int i = 0; i < c->frames.size(); i++)
				{
					if (c->frames[i].timestamp == std::stof(input.toAnsiString()))
					{
						c->frames.erase(c->frames.begin() + i);
					}
				}
				std::sort(
					c->frames.begin(),
					c->frames.end(),
					[](Skeleton::Animation::Frame a, Skeleton::Animation::Frame b)
					{
						return a.timestamp < b.timestamp;
					}
				);
			}
		}
	}
	if (enterCommandPage == PageTexture)
	{
		auto t = &skeleton.tex[currentTexture];
		if (enterCommandID == TextureID)
		{
			for (int i = 0; i < skeleton.tex.size(); i++)
			{
				if (skeleton.tex[i].name == input) { currentTexture = i; }
			}
		}
		if (enterCommandID == TextureName) { t->name = input; }
		if (enterCommandID == TexturePath) { t->path = input; t->tex.loadFromFile(input); }
		if (enterCommandID == TextureRect)
		{
			auto tr = tr::splitStr(input, " ");
			t->rect = {
				std::stoi(tr[0].toAnsiString()), std::stoi(tr[1].toAnsiString()),
				std::stoi(tr[2].toAnsiString()), std::stoi(tr[3].toAnsiString())
			};
		}
	}
	if (enterCommandPage == PageAnimation)
	{
		auto a = &skeleton.anims[currentAnimation];
		if (enterCommandID == AnimationID) { currentAnimation = tr::clamp(std::stoi(input.toAnsiString()), 0, skeleton.anims.size() - 1); }
		if (enterCommandID == AnimationName) { a->name = input; }
		if (enterCommandID == AnimationDuration) { a->duration = std::stof(input.toAnsiString()); }
		if (enterCommandID == AnimationRepeat) { a->repeat = std::stoi(input.toAnsiString()); }
		if (enterCommandID == AnimationCopyStart)
		{
			Skeleton::Animation *a = nullptr;
			for (int i = 0; i < skeleton.anims.size(); i++)
			{
				if (skeleton.anims[i].name == input) { a = &skeleton.anims[i]; break; }
			}
			if (!a) return;
			auto anim = skeleton.getCurrentAnim();
			anim->changes.clear();
			for (int i = 0; i < a->changes.size(); i++)
			{
				auto c = a->changes[i];
				c.frames.erase(c.frames.begin() + 1, c.frames.end());
				anim->changes.push_back(c);
			}
		}
		if (enterCommandID == AnimationCopyEnd)
		{
			Skeleton::Animation *a = nullptr;
			for (int i = 0; i < skeleton.anims.size(); i++)
			{
				if (skeleton.anims[i].name == input) { a = &skeleton.anims[i]; break; }
			}
			if (!a) return;
			auto anim = skeleton.getCurrentAnim();
			anim->changes.clear();
			for (int i = 0; i < a->changes.size(); i++)
			{
				auto c = a->changes[i];
				c.frames.erase(c.frames.begin(), c.frames.end() - 1);
				c.frames[0].timestamp = 0;
				if (c.frames[0].layer == -1) c.frames[0].layer = a->changes[i].frames[0].layer;
				if (c.frames[0].origin == sf::Vector2f(-1, -1)) c.frames[0].origin = a->changes[i].frames[0].origin;
				if (c.frames[0].root == -1) c.frames[0].root = a->changes[i].frames[0].root;
				if (c.frames[0].tex.isEmpty()) c.frames[0].tex = a->changes[i].frames[0].tex;
				anim->changes.push_back(c);
			}
		}
	}
	if (enterCommandPage == PageFrame)
	{
		auto f = skeleton.getCurrentAnim()->getChanger(currentBone)->getFrame(currentFrame);
		if (enterCommandID == FrameID) { currentBone = tr::clamp(std::stoi(input.toAnsiString()), 0, skeleton.bones.size()); }
		if (enterCommandID == FrameTS)
		{
			f->timestamp = std::stof(input.toAnsiString());
			std::sort(
				skeleton.getCurrentAnim()->getChanger(currentBone)->frames.begin(),
				skeleton.getCurrentAnim()->getChanger(currentBone)->frames.end(),
				[](Skeleton::Animation::Frame a, Skeleton::Animation::Frame b)
				{
					return a.timestamp < b.timestamp;
				}
			);
		}
		if (enterCommandID == FrameRotate) { f->r = std::stof(input.toAnsiString()); }
		if (enterCommandID == FrameTex) { f->tex = input; }
		if (enterCommandID == FrameLayer) { f->layer = std::stoi(input.toAnsiString()); }
		if (enterCommandID == FrameRoot) { f->root = std::stoi(input.toAnsiString()); }
		if (enterCommandID == FrameOrigin)
		{
			auto o = tr::splitStr(input, " ");
			f->origin = {
				std::stof(o[0].toAnsiString()),
				std::stof(o[1].toAnsiString())
			};
		}
	}
}

class Timeline
{
public:
	static void draw()
	{
		//Background
		sf::RectangleShape bg({window.getSize().x / 4 * 3, window.getSize().y / 4});
		bg.setPosition(0, window.getSize().y / 4 * 3);
		bg.setFillColor({64, 64, 64, 255});
		window.draw(bg);
		//Time line
		if (!skeleton.anims.size() || currentAnimation != tr::clamp(currentAnimation, 0, skeleton.anims.size())) return;
		sf::RectangleShape line({bg.getSize().x / 4 * 3, 10});
		line.setOrigin(line.getSize() / 2.0f);
		line.setPosition(window.getSize().x / 4 * 1.5, window.getSize().y / 4 * 3.5);
		window.draw(line);
		sf::Text tp("0", font, 18);
		tp.setPosition(line.getGlobalBounds().getPosition() + sf::Vector2f(0, 10));
		tp.setOrigin(tp.getLocalBounds().width / 2, 0);
		window.draw(tp);
		tp.setPosition(line.getGlobalBounds().getPosition() + line.getGlobalBounds().getSize());
		tp.setString(std::to_string(skeleton.anims[currentAnimation].duration));
		tp.setOrigin(tp.getLocalBounds().width / 2, 0);
		window.draw(tp);
		auto lr = line.getGlobalBounds();
		sf::RectangleShape point({10, 10});
		point.setOrigin(5, 0);
		point.setFillColor(sf::Color::Blue);
		point.setPosition(
			(int)tr::clamp(lr.left + currentFrame / skeleton.anims[currentAnimation].duration * lr.width, lr.left + 5, lr.left + lr.width - 5),
			lr.top
		);
		window.draw(point);
		tp.setString(std::to_string(currentFrame));
		tp.setPosition(point.getPosition().x, lr.top - 15);
		tp.setOrigin(tp.getLocalBounds().width / 2, tp.getLocalBounds().height);
		window.draw(tp);
		auto a = skeleton.getCurrentAnim();
		auto c = a->getChanger(currentBone);
		if (c != nullptr) for (int i = 0; i < c->frames.size(); i++)
		{
			point.setOrigin(0, 0);
			point.setPosition(
				(int)tr::clamp(lr.left + c->frames[i].timestamp / a->duration * lr.width, lr.left, lr.left + lr.width - 10),
				lr.top
			);
			point.setFillColor(sf::Color::Green);
			window.draw(point);
		}
		if (sf::FloatRect(lr.left - 15, lr.top - 15, lr.width + 30, lr.height + 30).contains(mouse))
		{
			//Hint
			sf::Text hint("Precision: Shift(0.00), Control(0.0), Alt(0)", font, 20);
			hint.setPosition(bg.getPosition());
			window.draw(hint);
			point.setOrigin(5, 0);
			point.setPosition(
				(int)tr::clamp(mouse.x, lr.left + 5, lr.left + lr.width - 5),
				lr.top
			);
			point.setFillColor(sf::Color::Red);
			window.draw(point);
			tp.setString(std::to_string(tr::clamp(mouse.x - lr.left, 0, lr.width) / lr.width * skeleton.anims[currentAnimation].duration));
			tp.setOrigin(tp.getLocalBounds().width / 2, 0);
			tp.setPosition(point.getPosition().x, lr.top + lr.height + 15);
			window.draw(tp);
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
			{
				auto value = std::to_string(tr::clamp(mouse.x - lr.left, 0, lr.width) / lr.width * skeleton.anims[currentAnimation].duration);
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
				{
					value = value.substr(0, value.find(".") + 3);
				}
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
				{
					value = value.substr(0, value.find(".") + 2);
				}
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt))
				{
					value = value.substr(0, value.find("."));
				}
				currentFrame = std::stof(value);
			}
		}
	}
};

int getNearestBone(sf::Vector2f pointer)
{
	auto rect = sf::FloatRect(0, 0, window.getSize().x / 4 * 3, window.getSize().y / 4 * 3);
	if (!rect.contains(mouse)) { return -1; }
	int nearest = -1;
	float dist = 100000;
	for (int i = 0; i < skeleton.bones.size(); i++)
	{
		auto b = skeleton.getBone(i);
		sf::Vector2f delta = pointer - b->getEnd(0.5);
		float d = sqrt(delta.x * delta.x + delta.y * delta.y);
		if (d < dist) { nearest = i; dist = d; } 
	}
	return nearest;
}

int main()
{
	window.create(sf::VideoMode(1024, 576), "Skeleton Editor"); window.setVerticalSyncEnabled(true);
	sf::RenderTexture screen;
	screen.create(window.getSize().x / 4 * 3, window.getSize().y / 4 * 3);
	sf::View camera = screen.getView();
	camera.setCenter(0, 0);

	for (auto p : std::filesystem::directory_iterator(std::filesystem::current_path()))
	{
		auto path = p.path().string();
		if (std::filesystem::exists(path + "/global/settings.trconf"))
		{
			path.erase(0, std::filesystem::current_path().string().length() + 1);
			font.loadFromFile(path + "/global/font.ttf");
		}
	}

	enter = {"", font, 20};
	enter.setFillColor(sf::Color::White);

	reloadUI();

	sf::Clock clock;

	float speed = 200;
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) { window.close(); }
			else if (event.type == sf::Event::Resized)
			{
				window.setView(sf::View({0, 0, event.size.width, event.size.height}));
				screen.create(event.size.width / 4 * 3, event.size.height / 4 * 3);
				camera.setSize(event.size.width / 4 * 3, event.size.height / 4 * 3);
				speed = 200;
			}
			else if (event.type == sf::Event::MouseButtonPressed)
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					for (int i = 0; i < ui.size(); i++)
					{
						if (ui[i].getGlobalBounds().contains({
							event.mouseButton.x,
							event.mouseButton.y
						}))
						{
							execute(currentPage, i);
							std::cout << currentPage << "|" << i << std::endl;
						}
					}
				}
			}
			else if (event.type == sf::Event::TextEntered && typing)
			{
				auto code = event.text.unicode;
				if (code != 27 && code != 13 && code != 9 && code != 124 && code != 8) enter.setString(enter.getString() + sf::String(code));
				if (code == 8)
				{
					auto str = enter.getString();
					if (str.toAnsiString()[str.getSize() - 1] != '|') { enter.setString(str.substring(0, str.getSize() - 1)); }
				}
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape) { if (typing) { enter.setString(""); typing = false; } }
				if (event.key.code == sf::Keyboard::Enter && typing)
				{
					executeEnter();
					typing = false;
					enter.setString("");
				}
				if (event.key.code == sf::Keyboard::Num1 && !typing) { currentPage = PageCreate; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num2 && !typing) { currentPage = PageBone; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num3 && !typing) { currentPage = PageTexture; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num4 && !typing) { currentPage = PageAnimation; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num5 && !typing) { currentPage = PageFrame; reloadUI(); }
				if (event.key.code == sf::Keyboard::S && event.key.control && !typing) { skeleton.saveAs(saveWindow()); }
				if (event.key.code == sf::Keyboard::O && event.key.control && !typing) { skeleton.loadFromFile(openWindow()); }
				if (event.key.code == sf::Keyboard::Q && !typing) { currentFrame = 0; }
				if (event.key.code == sf::Keyboard::Space && !typing) { play = !play; }
			}
			else if (event.type == sf::Event::MouseWheelScrolled)
			{
				auto zoom = event.mouseWheelScroll.delta;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
				{
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) zoom *= 2;
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) zoom *= 5;
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) zoom *= 10;
					if (auto a = skeleton.getCurrentAnim())
					if (auto c = a->getChanger(currentBone))
					if (auto f = c->getFrame(currentFrame)) f->r += zoom;
				}
				else
				{
					if (zoom == -1) { camera.zoom(0.5); speed /= 2; }
					if (zoom == 1) { camera.zoom(2); speed *= 2; }
				}
			}
		}
		deltaTime = clock.restart().asSeconds();
		mouse = (sf::Vector2f)sf::Mouse::getPosition(window);

		if (window.hasFocus())
		{
			float coff = 1;
			if (!typing && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) coff *= 2;
			if (!typing && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) coff /= 2;
			if (!typing && sf::Keyboard::isKeyPressed(sf::Keyboard::W)) camera.move(0, -speed * coff * deltaTime);
			if (!typing && sf::Keyboard::isKeyPressed(sf::Keyboard::S)) camera.move(0, speed * coff * deltaTime);
			if (!typing && sf::Keyboard::isKeyPressed(sf::Keyboard::A)) camera.move(-speed * coff * deltaTime, 0);
			if (!typing && sf::Keyboard::isKeyPressed(sf::Keyboard::D)) camera.move(speed * coff * deltaTime, 0);
		}

		skeleton.update();
		
		nearest = getNearestBone(screen.mapPixelToCoords((sf::Vector2i)mouse));
		if (nearest != -1)
		if (window.hasFocus())
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		if (skeleton.bones[nearest].spr.getGlobalBounds().contains(screen.mapPixelToCoords((sf::Vector2i)mouse)) ||
			skeleton.bones[nearest].spr.getLocalBounds().getSize() == sf::Vector2f(0, 0))
		{
			currentBone = nearest;
			currentPage = PageBone;
			reloadUI();
		}

		window.clear();
		screen.clear({127, 127, 127, 255});
		skeleton.draw(&screen);
		skeleton.drawBones(&screen);
		screen.setView(camera);
		screen.display(); window.draw(sf::Sprite(screen.getTexture()));
		updateUI();
		float xPos = window.getSize().x / 4 * 3;
		for (int i = 0; i < ui.size(); i++)
		{
			if (i == 0) ui[i].setPosition(xPos, 0);
			else ui[i].setPosition(xPos, ui[i - 1].getPosition().y + ui[i - 1].getGlobalBounds().height + 4);
			ui[i].setFillColor(sf::Color::White);
			if (ui[i].getGlobalBounds().contains(mouse)) ui[i].setFillColor(sf::Color::Red);
			window.draw(ui[i]);
		}
		enter.setPosition(xPos, window.getSize().y - 2);
		enter.setOrigin(0, enter.getGlobalBounds().height);
		window.draw(enter);
		Timeline::draw();
		window.display();
	}
}