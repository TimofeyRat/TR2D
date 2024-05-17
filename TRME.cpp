#include <SFML/Graphics.hpp>
#include <windows.h>
#include <filesystem>
#include <iostream>
#include "Engine/hGlobal.hpp"
#include <pugixml.hpp>
#include "Engine/hAnimation.hpp"
#include "Engine/hProgrammable.hpp"

sf::RenderWindow window;
sf::RenderTexture screen;
float deltaTime;
sf::Font font;
int currentLevel,
	currentPage,
	currentTile,
	currentEnt,
	currentTrigger,
	currentCtrl,
	currentLight;
sf::Vector2f mouse;
sf::Text enter;
bool typing;
sf::String assets, currentPath;
sf::Vector2i command;

struct Level
{
	struct Map
	{
		sf::String tilePath;
		sf::Texture tileTex;
		sf::Vector2i tileSize;
		std::vector<sf::IntRect> rects;
		std::vector<std::vector<sf::Uint16>> tiles;
		float scale;
		sf::Vector2i mapSize;
		Map()
		{
			tilePath = "";
			tileTex = sf::Texture();
			tileSize = {0, 0};
			rects.clear();
			tiles.clear();
			scale = 1;
			mapSize = {0, 0};
		}
		void generateTiles()
		{
			if (!tileSize.x || !tileSize.y) return;
			rects.clear();
			const auto texX = tileTex.getSize().x / tileSize.x,
				texY = tileTex.getSize().y / tileSize.y;
			for (short id = 0; id < texX * texY; id++)
			{
				sf::IntRect r;
				r.left = (int)(id % texX) * tileSize.x;
				r.top = (int)(id / texY) * tileSize.y;
				r.width = tileSize.x;
				r.height = tileSize.y;
				rects.push_back(r);
			}
		}
		void resize(int x, int y)
		{
			tiles.resize(x);
			for (int i = 0; i < x; i++)
			{
				tiles[i].resize(y);
			}
			mapSize = {x, y};
		}
		void draw()
		{
			sf::RectangleShape bounds({mapSize.x * tileSize.x * scale, mapSize.y * tileSize.y * scale});
			bounds.setFillColor({0, 0, 0, 0});
			bounds.setOutlineColor(sf::Color::Green);
			bounds.setOutlineThickness(2);
			screen.draw(bounds);
			sf::Sprite tile(tileTex);
			tile.setScale(scale, scale);
			for (int x = 0; x < mapSize.x; x++)
			{
				for (int y = 0; y < mapSize.y; y++)
				{
					auto id = tiles[x][y];
					if (!id || id != tr::clamp(id, 0, rects.size() - 1)) continue;
					tile.setTextureRect(rects[id - 1]);
					tile.setPosition(x * tileSize.x * scale, y * tileSize.y * scale);
					screen.draw(tile);
				}
			}
		}
	} map;
	struct Spawner
	{
		sf::Vector2f position;
		sf::String name;
		sf::Texture tex;
		sf::Sprite spr;
		Spawner()
		{
			position = {0, 0};
			name = "";
			tex = sf::Texture();
			spr = sf::Sprite();
		}
		Spawner(sf::Vector2f pos, sf::String id)
		{
			position = pos;
			name = id;
			for (auto p : std::filesystem::recursive_directory_iterator(assets.toAnsiString() + "ents/"))
			{
				pugi::xml_document ent;
				auto path = p.path().string();
				ent.load_file(pugi::as_wide(path).c_str());
				if (sf::String(ent.first_child().attribute(L"name").as_string()) == name)
				{
					Skeleton s; s.loadFromFile(pugi::as_utf8(ent.first_child().attribute(L"skeleton").as_string()));
					tex.loadFromImage(s.getTexture("headIdle")->tex->copyToImage());
					spr.setTexture(tex);
					spr.setTextureRect(s.getTexture("headIdle")->rect);
					spr.setOrigin(spr.getTextureRect().width / 2, spr.getTextureRect().height / 2);
					return;
				}
			}
		}
	};
	struct Trigger : public Programmable
	{
		sf::FloatRect rect;
		sf::RectangleShape hitbox;
		Trigger()
		{
			rect = {0, 0, 0, 0};
			hitbox = sf::RectangleShape();
			clear();
		}
		Trigger(sf::String prompt)
		{
			clear();
			for (auto cmd : tr::splitStr(prompt, ";"))
			{
				auto args = tr::splitStr(cmd, "=");
				if (tr::strContains(args[1], "str"))
				{
					setVar(args[0], args[2]);
				}
				else if (tr::strContains(args[1], "num"))
				{
					setVar(args[0], std::stof(args[2].toAnsiString()));
				}
			}
			auto pos = tr::splitStr(getVar("pos"), " ");
			auto size = tr::splitStr(getVar("size"), " ");
			rect.left = std::stof(pos[0].toAnsiString()); rect.top = std::stof(pos[1].toAnsiString());
			rect.width = std::stof(size[0].toAnsiString()); rect.height = std::stof(size[1].toAnsiString());
			setVar("x", rect.left); setVar("y", rect.top); setVar("w", rect.width); setVar("h", rect.height);
			hitbox.setSize(rect.getSize());
			hitbox.setRotation(getVar("angle"));
			hitbox.setOrigin(rect.getSize() / 2.0f);
			hitbox.setPosition(rect.getPosition() + rect.getSize() / 2.0f);
			hitbox.setFillColor({0, 0, 0, 0});
			hitbox.setOutlineColor(sf::Color::Red);
			hitbox.setOutlineThickness(-2);
		}
		void update()
		{
			hitbox.setSize(rect.getSize());
			hitbox.setRotation(getVar("angle"));
			hitbox.setOrigin(rect.getSize() / 2.0f);
			hitbox.setPosition(rect.getPosition() + rect.getSize() / 2.0f);
		}
	};
	struct Control
	{
		sf::String name, ctrl;
		Control() { name = ctrl = ""; }
		Control(sf::String id, sf::String input) { name = id; ctrl = input; }
	};
	struct Light
	{
		sf::Vector2f pos;
		sf::Color clr;
		float radius, angle, field;
		Light()
		{
			pos = {0, 0};
			clr = sf::Color::White;
			radius = angle = field = 0;
		}
		Light(sf::Vector2f position, sf::Color color, float r, float a, float f)
		{
			pos = position;
			clr = color;
			radius = r;
			angle = a;
			field = f;
		}
	};
	sf::String name;
	sf::Texture bgTex;
	sf::String bgPath, camOwner;
	sf::FloatRect bgBounds;
	sf::String music;
	float musicVolume;
	sf::FloatRect camera;
	std::vector<Spawner> ents;
	std::vector<Trigger> triggers;
	std::vector<Control> ctrl;
	std::vector<Light> lights;
	sf::Vector2f gravity;
	Level()
	{
		map = Map();
		name = "";
		bgTex = sf::Texture();
		bgBounds = {0, 0, 0, 0};
		music = "";
		camera = {0, 0, 0, 0};
		ents.clear();
		triggers.clear();
		ctrl.clear();
		gravity = {0, 0};
		bgPath = "";
		camOwner = "";
	}
	void draw()
	{
		sf::Sprite bg(bgTex);
		bg.setPosition(bgBounds.getPosition());
		bg.scale(
			bgBounds.width / bg.getLocalBounds().width,
			bgBounds.height / bg.getLocalBounds().height
		);
		screen.draw(bg);
		map.draw();
		for (int i = 0; i < triggers.size(); i++)
		{
			triggers[i].update();
			screen.draw(triggers[i].hitbox);
		}
		for (int i = 0; i < lights.size(); i++)
		{
			sf::VertexArray va(sf::LinesStrip, 34);
			auto *l = &lights[i];
			va[0] = {l->pos, l->clr};
			float delta = l->field / 32;
			for (int p = 1; p <= 33; p++)
			{
				va[tr::clamp(p, 0, 32)] = {l->pos + tr::getDelta(l->angle - l->field / 2 + delta * (p - 1)) * l->radius, l->clr};
			}
			va[33] = va[0];
			screen.draw(va);
		}
		sf::RectangleShape cam(camera.getPosition());
		cam.setPosition(camera.getSize());
		cam.setFillColor({0, 0, 0, 0});
		cam.setOutlineColor(sf::Color::Green);
		cam.setOutlineThickness(-2);
		screen.draw(cam);
		for (int i = 0; i < ents.size(); i++)
		{
			ents[i].spr.setPosition(ents[i].position);
			ents[i].spr.setTexture(ents[i].tex);
			ents[i].spr.setScale(2, 2);
			screen.draw(ents[i].spr);
		}
	}
};

std::vector<Level> lvls;

void loadFromFile(sf::String path)
{
	lvls.clear();
	pugi::xml_document doc;
	doc.load_file(path.toWideString().c_str());
	for (auto lvl : doc.children())
	{
		Level level;
		level.name = lvl.attribute(L"name").as_string();
		auto map = lvl.child(L"map");
		level.map.tilePath = map.attribute(L"tex").as_string();
		level.map.tileTex.loadFromFile(level.map.tilePath);
		auto ts = tr::splitStr(map.attribute(L"tileSize").as_string(L"0 0"), " ");
		level.map.tileSize = {
			std::stoi(ts[0].toAnsiString()),
			std::stoi(ts[1].toAnsiString())
		};
		level.map.generateTiles();
		auto ms = tr::splitStr(map.attribute(L"size").as_string(L"0 0"), " ");
		level.map.resize(std::stoi(ms[0].toAnsiString()), std::stoi(ms[1].toAnsiString()));
		level.map.scale = map.attribute(L"scale").as_float(1);
		auto tilemap = tr::splitStr(map.text().as_string(), "x");
		for (int y = 0; y < level.map.mapSize.y; y++)
		{
			for (int x = 0; x < level.map.mapSize.x; x++)
			{
				level.map.tiles[x][y] = std::stoi(tilemap[y * level.map.mapSize.x + x].toAnsiString());
			}
		}
		level.bgPath =pugi::as_utf8(lvl.child(L"background").attribute(L"path").as_string());
		level.bgTex.loadFromFile(level.bgPath);
		auto bgBounds = tr::splitStr(lvl.child(L"background").attribute(L"bounds").as_string(L"0 0 0 0"), " ");
		level.bgBounds = {
			std::stof(bgBounds[0].toAnsiString()), std::stof(bgBounds[1].toAnsiString()),
			std::stof(bgBounds[2].toAnsiString()), std::stof(bgBounds[3].toAnsiString())
		};
		level.music = lvl.child(L"music").attribute(L"path").as_string();
		level.musicVolume = lvl.child(L"music").attribute(L"volume").as_float(100);
		auto camSize = tr::splitStr(lvl.child(L"camera").attribute(L"size").as_string(L"0 0"), " ");
		auto camOS = tr::splitStr(lvl.child(L"camera").attribute(L"offset").as_string(L"0 0"), " ");
		level.camera = {
			std::stof(camSize[0].toAnsiString()), std::stof(camSize[1].toAnsiString()),
			std::stof(camOS[0].toAnsiString()), std::stof(camOS[1].toAnsiString())
		};
		level.camOwner = lvl.child(L"camera").attribute(L"owner").as_string();
		auto gravity = tr::splitStr(lvl.child(L"gravity").attribute(L"value").as_string(L"0 0"), " ");
		level.gravity = {
			std::stof(gravity[0].toAnsiString()),
			std::stof(gravity[1].toAnsiString())
		};
		for (auto ent : lvl.children(L"entity"))
		{
			auto pos = tr::splitStr(ent.attribute(L"pos").as_string(), " ");
			level.ents.push_back(Level::Spawner(
				{std::stof(pos[0].toAnsiString()), std::stof(pos[1].toAnsiString())},
				ent.attribute(L"name").as_string()
			));
		}
		for (auto trigger : lvl.children(L"trigger"))
		{
			sf::String prompt;
			for (auto attr: trigger.attributes())
			{
				auto name = tr::splitStr(attr.name(), "_");
				if (name[1] == "str")
				{
					prompt += name[0] + "=str=\"" + attr.as_string() + "\";";
				}
				if (name[1] == "num")
				{
					prompt += name[0] + "=num=" + attr.as_string() + ";";
				}
			}
			level.triggers.push_back(Level::Trigger(prompt));
		}
		for (auto c : lvl.children(L"control"))
		{
			level.ctrl.push_back(Level::Control(
				c.attribute(L"ent").as_string(),
				c.attribute(L"controller").as_string()
			));
		}
		for (auto l : lvl.children(L"light"))
		{
			auto pos = tr::splitStr(l.attribute(L"pos").as_string(), " ");
			auto clr = tr::splitStr(l.attribute(L"color").as_string(), " ");
			level.lights.push_back(Level::Light(
				{std::stof(pos[0].toAnsiString()), std::stof(pos[1].toAnsiString())},
				{
					std::stof(clr[0].toAnsiString()), std::stof(clr[1].toAnsiString()),
					std::stof(clr[2].toAnsiString()), std::stof(clr[3].toAnsiString())
				},
				l.attribute(L"radius").as_float(),
				l.attribute(L"angle").as_float(),
				l.attribute(L"field").as_float()
			));
		}
		lvls.push_back(level);
	}
}

void save(sf::String file)
{
	pugi::xml_document doc;
	for (int i = 0; i < lvls.size(); i++)
	{
		auto level = doc.append_child(L"level");
		level.append_attribute(L"name") = lvls[i].name.toWideString().c_str();
		//Map node
		auto *m = &lvls[i].map;
		auto map = level.append_child(L"map");
		map.set_name(L"map");
		map.append_attribute(L"size") = pugi::as_wide(
			std::to_string(m->mapSize.x) + " " +
			std::to_string(m->mapSize.y)
		).c_str();
		map.append_attribute(L"tex") = pugi::as_wide(m->tilePath).c_str();
		map.append_attribute(L"scale") = m->scale;
		map.append_attribute(L"tileSize") = pugi::as_wide(
			std::to_string(m->tileSize.x) + " " +
			std::to_string(m->tileSize.y)
		).c_str();
		sf::String tilemap;
		for (int y = 0; y < m->mapSize.y; y++)
		{
			for (int x = 0; x < m->mapSize.x; x++)
			{
				tilemap += std::to_string(m->tiles[x][y]) + "x";
			}
		}
		map.text() = tilemap.substring(0, tilemap.getSize() - 1).toWideString().c_str();
		//Background node
		auto bg = level.append_child(L"background");
		bg.append_attribute(L"path") = lvls[i].bgPath.toWideString().c_str();
		bg.append_attribute(L"bounds") = pugi::as_wide(
			std::to_string(lvls[i].bgBounds.left) + " " + std::to_string(lvls[i].bgBounds.top) + " " +
			std::to_string(lvls[i].bgBounds.width) + " " + std::to_string(lvls[i].bgBounds.height)
		).c_str();
		//Music node
		level.append_child(L"music").append_attribute(L"path") = lvls[i].music.toWideString().c_str();
		level.child(L"music").append_attribute(L"volume") = lvls[i].musicVolume;
		//Camera node
		auto cam = level.append_child(L"camera");
		cam.append_attribute(L"size") = pugi::as_wide(
			std::to_string(lvls[i].camera.left) + " " +
			std::to_string(lvls[i].camera.top)
		).c_str();
		cam.append_attribute(L"offset") = pugi::as_wide(
			std::to_string(lvls[i].camera.width) + " " +
			std::to_string(lvls[i].camera.height)
		).c_str();
		cam.append_attribute(L"owner") = lvls[i].camOwner.toWideString().c_str();
		//Gravity
		level.append_child(L"gravity").append_attribute(L"value") = pugi::as_wide(
			std::to_string(lvls[i].gravity.x) + " " +
			std::to_string(lvls[i].gravity.y)
		).c_str();
		//Triggers
		for (int j = 0; j < lvls[i].triggers.size(); j++)
		{
			auto *t = &lvls[i].triggers[j];
			auto trigger = level.append_child(L"trigger");
			for (auto v : t->getVars())
			{
				if (v.str.isEmpty())
				{
					trigger.append_attribute(sf::String(v.name + "_num").toWideString().c_str()) = v.num;
				}
				else
				{
					trigger.append_attribute(sf::String(v.name + "_str").toWideString().c_str()) = v.str.toWideString().c_str();
				}
			}
		}
		//Spawners
		for (int j = 0; j < lvls[i].ents.size(); j++)
		{
			auto *s = &lvls[i].ents[j];
			auto spawner = level.append_child(L"entity");
			spawner.append_attribute(L"name") = s->name.toWideString().c_str();
			spawner.append_attribute(L"pos") = pugi::as_wide(
				std::to_string(s->position.x) + " " +
				std::to_string(s->position.y)
			).c_str();
		}
		for (int j = 0; j < lvls[i].ctrl.size(); j++)
		{
			auto *c = &lvls[i].ctrl[j];
			auto ctrl = level.append_child(L"control");
			ctrl.append_attribute(L"ent") = c->name.toWideString().c_str();
			ctrl.append_attribute(L"controller") = c->ctrl.toWideString().c_str();
		}
		for (int j = 0; j < lvls[i].lights.size(); j++)
		{
			auto *l = &lvls[i].lights[j];
			auto light = level.append_child(L"light");
			light.append_attribute(L"pos") = sf::String(
				std::to_string(l->pos.x) + " " + std::to_string(l->pos.y)
			).toWideString().c_str();
			light.append_attribute(L"color") = sf::String(
				std::to_string(l->clr.r) + " " + std::to_string(l->clr.g) + " " +
				std::to_string(l->clr.b) + " " + std::to_string(l->clr.a)
			).toWideString().c_str();
			light.append_attribute(L"radius") = l->radius;
			light.append_attribute(L"angle") = l->angle;
			light.append_attribute(L"field") = l->field;
		}
	}
	doc.save_file(pugi::as_wide(file).c_str());
}

#define PageMain 0
#define PageDraw 1
#define PageEnt 2
#define PageTrigger 3
#define PageControl 4
#define PageLight 5

//Main page
#define MainOpen 0
#define MainSave 1
#define MainNew 2
#define MainDelete 3
#define MainName 4
#define MainLevels 5
#define MainBG 6
#define MainBounds 7
#define MainMusic 8
#define MainVolume 9
#define MainGravity 10
#define MainCamera 11

//Drawing page
#define DrawPixelSize 0
#define DrawMap 1
#define DrawTex 2
#define DrawTile 3
#define DrawScale 4

//Spawners page
#define Ents 0
#define EntNew 1
#define EntDelete 2
#define EntName 3
#define EntPos 4

//Triggers page
#define Triggers 0
#define TriggerNew 1
#define TriggerDelete 2
#define TriggerName 3
#define TriggerVars 4

//Controls page
#define Controls 0
#define ControlNew 1
#define ControlDelete 2
#define ControlEnt 3
#define ControlInput 4

//Lights page
#define Lights 0
#define LightNew 1
#define LightDelete 2
#define LightID 3
#define LightPos 4
#define LightClr 5
#define LightRadius 6
#define LightAngle 7
#define LightField 8

sf::String openWindow(char* filter = "TR2D World (*.trworld)\0*.trworld\0")
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
		while (tr::strContains(currentPath, "\\")) currentPath.replace("\\", "/");
		while (tr::strContains(file, "\\")) file.replace("\\", "/");
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
	ofn.lpstrFilter = "TR2D World (*.trworld)\0*.trworld\0";
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
		currentPath = path.string();
		while (tr::strContains(currentPath, "\\")) currentPath.replace("\\", "/");
		while (tr::strContains(file, "\\")) file.replace("\\", "/");
		return file;
	}
	return "Fail"; 
}

std::vector<sf::Text> ui;

void reloadUI()
{
	ui.clear();
	if (currentPage == PageMain) { ui.resize(12, {"", font, 20}); }
	if (currentPage == PageDraw) { ui.resize(5, {"", font, 20});}
	if (currentPage == PageEnt) { ui.resize(5, {"", font, 20}); }
	if (currentPage == PageTrigger) { ui.resize(5, {"", font, 20}); }
	if (currentPage == PageControl) { ui.resize(5, {"", font, 20}); }
	if (currentPage == PageLight) { ui.resize(9, {"", font, 20}); }
}

void updateUI()
{
	if (currentPage == PageMain)
	{
		ui[0].setString("Open world");
		ui[1].setString("Save world");
		ui[2].setString("New level");
		ui[3].setString("Delete level");
		if (!lvls.size()) return;
		auto l = &lvls[currentLevel];
		ui[4].setString("Level name:\n" + l->name);
		ui[5].setString("Level count: " + std::to_string(lvls.size()));
		ui[6].setString("Background path:\n" + l->bgPath);
		ui[7].setString("Background bounds:\n" +
			std::to_string((int)l->bgBounds.left) + " " +
			std::to_string((int)l->bgBounds.top) + " " +
			std::to_string((int)l->bgBounds.width) + " " +
			std::to_string((int)l->bgBounds.height)
		);
		ui[8].setString("Music path:\n" + l->music);
		ui[9].setString("Music volume: " + std::to_string((int)l->musicVolume));
		ui[10].setString("Gravity: " +
			std::to_string(l->gravity.x) + " " +
			std::to_string(l->gravity.y)
		);
		ui[11].setString("Camera size/offset:\n" +
			std::to_string((int)l->camera.left) + " " +
			std::to_string((int)l->camera.top) + " " +
			std::to_string((int)l->camera.width) + " " +
			std::to_string((int)l->camera.height)
		);
	}
	if (currentPage == PageDraw)
	{
		if (!lvls.size()) { currentPage = PageMain; reloadUI(); return; }
		auto l = &lvls[currentLevel];
		ui[0].setString("Pixel size:\n" +
			std::to_string(l->map.mapSize.x * l->map.tileSize.x * l->map.scale) + " " +
			std::to_string(l->map.mapSize.y * l->map.tileSize.y * l->map.scale)
		);
		ui[1].setString("Map size:\n" +
			std::to_string(l->map.mapSize.x) + " " +
			std::to_string(l->map.mapSize.y)
		);
		ui[2].setString("Tile texture:\n" + l->map.tilePath);
		ui[3].setString("Tile size:\n" +
			std::to_string(l->map.tileSize.x) + " " +
			std::to_string(l->map.tileSize.y)
		);
		ui[4].setString("Scale: " + std::to_string(l->map.scale));
	}
	if (currentPage == PageEnt)
	{
		if (!lvls.size()) { currentPage = PageMain; reloadUI(); return; }
		auto l = &lvls[currentLevel];
		ui[0].setString("Spawners count: " + std::to_string(l->ents.size()));
		ui[1].setString("New spawner");
		ui[2].setString("Delete spawner");
		if (!l->ents.size()) return;
		auto e = &l->ents[currentEnt];
		ui[3].setString("Entity:\n" + e->name);
		ui[4].setString("Position:\n" +
			std::to_string(e->position.x) + " " +
			std::to_string(e->position.y)
		);
	}
	if (currentPage == PageTrigger)
	{
		if (!lvls.size()) { currentPage = PageMain; reloadUI(); return; }
		auto l = &lvls[currentLevel];
		ui[0].setString("Triggers count: " + std::to_string(l->triggers.size()));
		ui[1].setString("New trigger");
		ui[2].setString("Delete trigger");
		if (!l->triggers.size()) return;
		auto t = &l->triggers[currentTrigger];
		ui[3].setString("Trigger name:\n" + t->getVar("name").str);
		sf::String vars = "Trigger vars:\n";
		auto v = t->getVars();
		for (int i = 0; i < v.size(); i++)
		{
			if (v[i].str == "name") continue;
			if (!v[i].str.isEmpty())
			{
				vars += v[i].name + "(str): " + v[i].str + "\n";
			}
			vars += v[i].name + "(num): " + std::to_string(v[i].num) + "\n";
		}
		vars.erase(vars.getSize() - 1);
		ui[4].setString(vars);
	}
	if (currentPage == PageControl)
	{
		if (!lvls.size()) { currentPage = PageMain; reloadUI(); return; }
		auto l = &lvls[currentLevel];
		ui[0].setString("Controls count: " + std::to_string(l->ctrl.size()));
		ui[1].setString("New control");
		ui[2].setString("Delete control");
		if (!l->ctrl.size()) return;
		auto c = &l->ctrl[currentCtrl];
		ui[3].setString("Entity:\n" + c->name);
		ui[4].setString("Controller:\n" + c->ctrl);
	}
	if (currentPage == PageLight)
	{
		if (!lvls.size()) { currentPage = PageMain; reloadUI(); return; }
		auto lvl = &lvls[currentLevel];
		ui[0].setString("Lights count: " + std::to_string(lvl->lights.size()));
		ui[1].setString("New light");
		ui[2].setString("Delete light");
		if (!lvl->lights.size()) return;
		auto l = &lvl->lights[currentLight];
		ui[3].setString("Current light: " + std::to_string(currentLight));
		ui[4].setString("Light position:\n" +
			std::to_string(l->pos.x) + " " +
			std::to_string(l->pos.y)
		);
		ui[5].setString("Light color:\n" +
			std::to_string((int)l->clr.r) + " " +
			std::to_string((int)l->clr.g) + " " +
			std::to_string((int)l->clr.b) + " " +
			std::to_string((int)l->clr.a)
		);
		ui[6].setString("Light radius: " + std::to_string(l->radius));
		ui[7].setString("Light angle: " + std::to_string(l->angle));
		ui[8].setString("Light field: " + std::to_string(l->field));
	}
}

void execute(int page, int btn)
{
	if (page == PageMain)
	{
		if (btn == MainOpen)
		{
			auto path = openWindow();
			if (path == "Fail") { return; }
			assets = path.substring(0, path.find("/") + 1);
			loadFromFile(path);
		}
		if (btn == MainSave)
		{
			auto path = saveWindow();
			if (path == "Fail") { return; }
			save(path);
		}
		if (btn == MainNew) { typing = true; enter.setString("New level name:\n"); }
		if (btn == MainDelete) { typing = true; enter.setString("Level name:\n"); }
		if (btn == MainName) { typing = true; enter.setString("New level name:\n"); }
		if (btn == MainLevels) { typing = true; enter.setString("Choose level by name/ID:\n"); }
		if (btn == MainBG)
		{
			auto path = openWindow("Texture (*.png)\0*.png\0");
			if (path == "Fail") return;
			lvls[currentLevel].bgPath = path;
			lvls[currentLevel].bgTex.loadFromFile(lvls[currentLevel].bgPath);
		}
		if (btn == MainBounds) { typing = true; enter.setString("Set background bounds:\n"); }
		if (btn == MainMusic)
		{
			auto path = openWindow("Music (*.ogg)\0*.ogg\0");
			if (path == "Fail") return;
			lvls[currentLevel].music = path;
		}
		if (btn == MainVolume) { typing = true; enter.setString("Set music volume:\n"); }
		if (btn == MainGravity) { typing = true; enter.setString("Set level gravity:\n"); }
		if (btn == MainCamera) { typing = true; enter.setString("Set new camera size or offset:\n"); }
	}
	if (page == PageDraw)
	{
		if (btn == DrawMap) { typing = true; enter.setString("Set map size:\n"); }
		if (btn == DrawTex)
		{
			auto path = openWindow("Texture (*.png)\0*.png\0");
			if (path == "Fail") return;
			lvls[currentLevel].map.tilePath = path;
			lvls[currentLevel].map.tileTex.loadFromFile(path);
			lvls[currentLevel].map.generateTiles();
		}
		if (btn == DrawTile) { typing = true; enter.setString("Set tile size:\n"); }
		if (btn == DrawScale) { typing = true; enter.setString("Set scale:\n"); }
	}
	if (page == PageEnt)
	{
		if (btn == Ents) { typing = true; enter.setString("Select entity by name/ID:\n"); }
		if (btn == EntNew) { typing = true; enter.setString("New entity name:\n"); }
		if (btn == EntDelete) { typing = true; enter.setString("Entity name:\n"); }
		if (btn == EntPos) { typing = true; enter.setString("Move entity to:\n"); }
	}
	if (page == PageTrigger)
	{
		if (btn == Triggers) { typing = true; enter.setString("Select trigger by name/ID:\n"); }
		if (btn == TriggerNew) { typing = true; enter.setString("New trigger name:\n"); }
		if (btn == TriggerDelete) { typing = true; enter.setString("Trigger name:\n"); }
		if (btn == TriggerName) { typing = true; enter.setString("Set trigger name:\n"); }
		if (btn == TriggerVars) { typing = true; enter.setString("Set variable value:\n"); }
	}
	if (page == PageControl)
	{
		if (btn == Controls) { typing = true; enter.setString("Select control by entity/ID:\n"); }
		if (btn == ControlNew) { typing = true; enter.setString("New control entity and input:\n"); }
		if (btn == ControlDelete) { typing = true; enter.setString("Control name/ID:\n"); }
	}
	if (page == PageLight)
	{
		if (btn == Lights || btn == LightID) { typing = true; enter.setString("Select light by ID:\n"); }
		if (btn == LightNew) { typing = true; enter.setString("Enter radius of light:\n"); }
		if (btn == LightDelete) { typing = true; enter.setString("Enter ID of light:\n"); }
		if (btn == LightPos) { typing = true; enter.setString("Set light position:\n"); }
		if (btn == LightClr) { typing = true; enter.setString("Set light color:\n"); }
		if (btn == LightRadius) { typing = true; enter.setString("Set light radius:\n"); }
		if (btn == LightAngle) { typing = true; enter.setString("Set light angle:\n"); }
		if (btn == LightField) { typing = true; enter.setString("Set light field:\n"); }
	}
	if (typing) command = {page, btn};
}

void cmd()
{
	sf::String prompt = enter.getString();
	prompt = prompt.substring(prompt.find("\n") + 1);
	auto page = command.x, btn = command.y;
	if (page == PageMain)
	{
		if (btn == MainNew)
		{
			Level l;
			l.name = prompt;
			lvls.push_back(l);
			currentLevel = lvls.size() - 1;
		}
		if (btn == MainDelete)
		{
			for (int i = 0; i < lvls.size(); i++)
			{
				if (lvls[i].name == prompt) { lvls.erase(lvls.begin() + i); break; }
			}
			currentLevel = tr::clamp(currentLevel, 0, lvls.size() - 1);
		}
		if (btn == MainName)
		{
			lvls[currentLevel].name = prompt;
		}
		if (btn == MainLevels)
		{
			if (std::isdigit(prompt.toAnsiString()[0]))
			{
				currentLevel = tr::clamp(std::stoi(prompt.toAnsiString()), 0, lvls.size() - 1);
			}
			else for (int i = 0; i < lvls.size(); i++)
			{
				if (lvls[i].name == prompt) { currentLevel = i; break; }
			}
		}
		if (btn == MainBounds)
		{
			auto b = tr::splitStr(prompt, " ");
			if (b.size() != 4) return;
			lvls[currentLevel].bgBounds = {
				std::stof(b[0].toAnsiString()), std::stof(b[1].toAnsiString()),
				std::stof(b[2].toAnsiString()), std::stof(b[3].toAnsiString())
			};
		}
		if (btn == MainVolume)
		{
			lvls[currentLevel].musicVolume = std::stof(prompt.toAnsiString());
		}
		if (btn == MainGravity)
		{
			auto g = tr::splitStr(prompt, " ");
			if (g.size() != 2) return;
			lvls[currentLevel].gravity = {std::stof(g[0].toAnsiString()), std::stof(g[1].toAnsiString())};
		}
		if (btn == MainCamera)
		{
			auto args = tr::splitStr(prompt, " ");
			if (args[0] == "size")
			{
				lvls[currentLevel].camera.left = std::stof(args[1].toAnsiString());
				lvls[currentLevel].camera.top = std::stof(args[2].toAnsiString());
			}
			else if (args[0] == "offset")
			{
				lvls[currentLevel].camera.width = std::stof(args[1].toAnsiString());
				lvls[currentLevel].camera.height = std::stof(args[2].toAnsiString());
			}
			else lvls[currentLevel].camera = {
				std::stof(args[0].toAnsiString()), std::stof(args[1].toAnsiString()),
				std::stof(args[2].toAnsiString()), std::stof(args[3].toAnsiString())
			};
		}
	}
	if (page == PageDraw)
	{
		if (btn == DrawMap)
		{
			auto s = tr::splitStr(prompt, " ");
			if (s.size() != 2) return;
			lvls[currentLevel].map.resize(
				std::stoi(s[0].toAnsiString()),
				std::stoi(s[1].toAnsiString())
			);
		}
		if (btn == DrawTile)
		{
			auto s = tr::splitStr(prompt, " ");
			if (s.size() != 2) return;
			lvls[currentLevel].map.tileSize = {
				std::stoi(s[0].toAnsiString()),
				std::stoi(s[1].toAnsiString())
			};
			lvls[currentLevel].map.generateTiles();
		}
		if (btn == DrawScale)
		{
			lvls[currentLevel].map.scale = std::stof(prompt.toAnsiString());
		}
	}
	if (page == PageEnt)
	{
		auto l = &lvls[currentLevel];
		if (btn == Ents)
		{
			if (std::isdigit(prompt.toAnsiString()[0]))
			{
				currentEnt = tr::clamp(std::stoi(prompt.toAnsiString()), 0, l->ents.size() - 1);
			}
			else for (int i = 0; i < l->ents.size(); i++)
			{
				if (l->ents[i].name == prompt)
				{
					currentEnt = i;
					break;
				}
			}
		}
		if (btn == EntNew)
		{
			l->ents.push_back(Level::Spawner({0, 0}, prompt));
			currentEnt = l->ents.size() - 1;
		}
		if (btn == EntDelete)
		{
			if (std::isdigit(prompt.toAnsiString()[0]))
			{
				l->ents.erase(l->ents.begin() + std::stoi(prompt.toAnsiString()));
			}
			else for (int i = 0; i < l->ents.size(); i++)
			{
				if (l->ents[i].name == prompt)
				{
					l->ents.erase(l->ents.begin() + i);
					break;
				}
			}
			currentEnt = tr::clamp(currentEnt, 0, l->ents.size() - 1);
		}
		if (btn == EntPos)
		{
			auto p = tr::splitStr(prompt, " ");
			if (p.size() != 2) return;
			l->ents[currentEnt].position = {
				std::stof(p[0].toAnsiString()),
				std::stof(p[1].toAnsiString())
			};
		}
	}
	if (page == PageTrigger)
	{
		auto l = &lvls[currentLevel];
		if (btn == Triggers)
		{
			if (std::isdigit(prompt.toAnsiString()[0]))
			{
				currentTrigger = tr::clamp(std::stoi(prompt.toAnsiString()), 0, l->triggers.size() - 1);
			}
			else for (int i = 0; i < l->triggers.size(); i++)
			{
				if (l->triggers[i].getVar("name") == prompt)
				{
					currentTrigger = i;
					break;
				}
			}
		}
		if (btn == TriggerNew)
		{
			Level::Trigger t;
			t.setVar("name", prompt);
			l->triggers.push_back(t);
			currentTrigger = l->triggers.size() - 1;
		}
		if (btn == TriggerDelete)
		{
			if (std::isdigit(prompt.toAnsiString()[0]))
			{
				l->triggers.erase(l->triggers.begin() + std::stoi(prompt.toAnsiString()));
			}
			else for (int i = 0; i < l->triggers.size(); i++)
			{
				if (l->triggers[i].getVar("name") == prompt)
				{
					currentTrigger = i;
					break;
				}
			}
			currentTrigger = tr::clamp(currentTrigger, 0, l->triggers.size() - 1);
		}
		if (btn == TriggerName)
		{
			l->triggers[currentTrigger].setVar("name", prompt);
		}
		if (btn == TriggerVars)
		{
			auto t = tr::splitStr(prompt, " ");
			if (std::isdigit(t[1].toAnsiString()[0]))
			{
				l->triggers[currentTrigger].setVar(t[0], std::stof(t[1].toAnsiString()));
			}
			else
			{
				l->triggers[currentTrigger].setVar(t[0], t[1]);
			}
		}
	}
	if (page == PageControl)
	{
		auto l = &lvls[currentLevel];
		if (btn == Controls)
		{
			if (std::isdigit(prompt.toAnsiString()[0]))
			{
				currentCtrl = tr::clamp(std::stoi(prompt.toAnsiString()), 0, l->ctrl.size() - 1);
			}
			else for (int i = 0; i < l->ctrl.size(); i++)
			{
				if (l->ctrl[i].name == prompt) { currentCtrl = i; break; }
			}
		}
		if (btn == ControlNew)
		{
			auto c = tr::splitStr(prompt, " ");
			if (c.size() != 2) return;
			l->ctrl.push_back(Level::Control(c[0], c[1]));
			currentEnt = l->ents.size() - 1;
		}
		if (btn == ControlDelete)
		{
			if (std::isdigit(prompt.toAnsiString()[0]))
			{
				l->ctrl.erase(l->ctrl.begin() + std::stoi(prompt.toAnsiString()));
			}
			else for (int i = 0; i < l->ctrl.size(); i++)
			{
				if (l->ctrl[i].name == prompt) { l->ctrl.erase(l->ctrl.begin() + i); break; }
			}
			currentCtrl = tr::clamp(currentCtrl, 0, l->ctrl.size() - 1);
		}
	}
	if (page == PageLight)
	{
		auto l = &lvls[currentLevel];
		if (btn == Lights || btn == LightID)
		{
			currentLight = tr::clamp(std::stoi(prompt.toAnsiString()), 0, l->lights.size() - 1);
		}
		if (btn == LightNew)
		{
			Level::Light light({0, 0}, sf::Color::White, std::stof(prompt.toAnsiString()), 0, 360);
			l->lights.push_back(light);
			currentLight = l->lights.size() - 1;
		}
		if (btn == LightDelete)
		{
			l->lights.erase(l->lights.begin() + std::stoi(prompt.toAnsiString()));
			currentLight = tr::clamp(currentLight, 0, l->lights.size());
		}
		if (btn == LightPos)
		{
			auto p = tr::splitStr(prompt, " ");
			if (p.size() != 2) return;
			l->lights[currentLight].pos = {
				std::stof(p[0].toAnsiString()),
				std::stof(p[1].toAnsiString())
			};
		}
		if (btn == LightClr)
		{
			auto p = tr::splitStr(prompt, " ");
			if (p.size() != 4) return;
			l->lights[currentLight].clr = {
				std::stof(p[0].toAnsiString()),
				std::stof(p[1].toAnsiString()),
				std::stof(p[2].toAnsiString()),
				std::stof(p[3].toAnsiString())
			};
		}
		if (btn == LightRadius)
		{
			l->lights[currentLight].radius = std::stof(prompt.toAnsiString());
		}
		if (btn == LightAngle)
		{
			l->lights[currentLight].angle = std::stof(prompt.toAnsiString());
		}
		if (btn == LightField)
		{
			l->lights[currentLight].field = std::stof(prompt.toAnsiString());
		}
	}
}

int main()
{
	window.create({1024, 576}, "Map Editor");
	window.setVerticalSyncEnabled(true);
	screen.create(window.getSize().x / 4 * 3, window.getSize().y);

	sf::View camera = sf::View({0, 0, screen.getSize().x, screen.getSize().y});

	for (auto p : std::filesystem::directory_iterator(std::filesystem::current_path()))
	{
		auto path = p.path().string();
		if (std::filesystem::exists(path + "/global/settings.trconf"))
		{
			path.erase(0, std::filesystem::current_path().string().length() + 1);
			font.loadFromFile(path + "/global/font.ttf");
		}
	}

	reloadUI();

	enter = {"", font, 20};

	sf::Clock clock;
	while (window.isOpen())
	{
		deltaTime = clock.restart().asSeconds();
		mouse = (sf::Vector2f)sf::Mouse::getPosition(window);
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
			if (event.type == sf::Event::Resized)
			{
				window.setView(sf::View({0, 0, event.size.width, event.size.height}));
				screen.create(event.size.width / 4 * 3, event.size.height);
				camera = sf::View({0, 0, screen.getSize().x, screen.getSize().y});
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				for (int i = 0; i < ui.size(); i++)
				{
					if (ui[i].getGlobalBounds().contains(mouse))
					{
						execute(currentPage, i);
					}
				}
			}
			if (event.type == sf::Event::TextEntered && typing)
			{
				auto code = event.text.unicode;
				if (code != 27 && code != 13 && code != 9 && code != 124 && code != 8) enter.setString(enter.getString() + sf::String(code));
				if (code == 8)
				{
					auto str = enter.getString();
					if (str.toAnsiString()[str.getSize() - 1] != '\n') { enter.setString(str.substring(0, str.getSize() - 1)); }
				}
			}
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape) { if (typing) enter.setString(""); typing = false; }
				if (event.key.code == sf::Keyboard::Enter && typing)
				{
					cmd();
					typing = false;
					enter.setString("");
				}
				if (event.key.code == sf::Keyboard::Num1 && !typing) { currentPage = PageMain; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num2 && !typing) { currentPage = PageDraw; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num3 && !typing) { currentPage = PageEnt; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num4 && !typing) { currentPage = PageTrigger; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num5 && !typing) { currentPage = PageControl; reloadUI(); }
				if (event.key.code == sf::Keyboard::Num6 && !typing) { currentPage = PageLight; reloadUI(); }
			}
			if (event.type == sf::Event::MouseWheelScrolled)
			{
				auto delta = event.mouseWheelScroll.delta;
				if (sf::FloatRect(0, 0, window.getSize().x / 4 * 3, window.getSize().y).contains(mouse) &&
					currentPage == PageDraw)
					currentTile = tr::clamp(currentTile + delta, 0, lvls[currentLevel].map.tiles.size() - 1);
			}
		}

		if (window.hasFocus() && !typing)
		{
			float speed = 250;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) { speed *= 2; }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) { speed /= 2; }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { camera.move(-speed * deltaTime, 0); }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { camera.move(speed * deltaTime, 0); }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { camera.move(0, -speed * deltaTime); }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { camera.move(0, speed * deltaTime); }

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) && lvls.size())
			{
				auto l = &lvls[currentLevel];
				auto point = screen.mapPixelToCoords((sf::Vector2i)mouse);
				if (currentPage == PageDraw)
				{
					sf::Vector2i tile = {
						tr::clamp(point.x / (l->map.tileSize.x * l->map.scale), 0, UINT16_MAX),
						tr::clamp(point.y / (l->map.tileSize.y * l->map.scale), 0, UINT16_MAX)
					};
					l->map.resize(tile.x, tile.y);
				}
				if (currentPage == PageEnt)
				{
					l->ents[currentEnt].position = point;
				}
				if (currentPage == PageTrigger)
				{
					l->triggers[currentTrigger].rect.left = point.x;
					l->triggers[currentTrigger].rect.top = point.y;
				}
				if (currentPage == PageLight)
				{
					l->lights[currentLight].pos = point;
				}
			}

			if (currentPage == PageDraw)
			{
				auto l = &lvls[currentLevel];
				if (sf::FloatRect(0, 0, window.getSize().x / 4 * 3, window.getSize().y).contains(mouse))
				{
					auto point = screen.mapPixelToCoords((sf::Vector2i)mouse);
					sf::Vector2i tile = {
						tr::clamp(point.x / (l->map.tileSize.x * l->map.scale), 0, l->map.mapSize.x - 1),
						tr::clamp(point.y / (l->map.tileSize.y * l->map.scale), 0, l->map.mapSize.y - 1)
					};
					if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
					{
						l->map.tiles[tile.x][tile.y] = currentTile;
					}
				}
			}
		}
		
		window.clear();
		screen.setView(camera);
		screen.clear({127, 127, 127});
		if (lvls.size())
		{
			auto *lvl = &lvls[currentLevel];
			lvl->draw();
			if (sf::FloatRect(0, 0, window.getSize().x / 4 * 3, window.getSize().y).contains(mouse) &&
				currentPage == PageDraw &&
				currentTile > 0)
			{
				sf::Sprite spr(lvl->map.tileTex, lvl->map.rects[currentTile - 1]);
				auto point = screen.mapPixelToCoords((sf::Vector2i)mouse);
				sf::Vector2i tile = {
					tr::clamp(point.x / (lvl->map.tileSize.x * lvl->map.scale), 0, lvl->map.mapSize.x - 1),
					tr::clamp(point.y / (lvl->map.tileSize.y * lvl->map.scale), 0, lvl->map.mapSize.y - 1)
				};
				spr.setPosition(
					tile.x * lvl->map.tileSize.x * lvl->map.scale,
					tile.y * lvl->map.tileSize.y * lvl->map.scale
				);
				screen.draw(spr);
			}
		}
		screen.display();
		window.draw(sf::Sprite(screen.getTexture()));
		updateUI();
		for (int i = 0; i < ui.size(); i++)
		{
			if (i == 0) ui[i].setPosition(window.getSize().x / 4 * 3, 0);
			else ui[i].setPosition(window.getSize().x / 4 * 3, ui[i - 1].getPosition().y + ui[i - 1].getGlobalBounds().height);
			if (ui[i].getGlobalBounds().contains(mouse)) { ui[i].setFillColor(sf::Color::Red); }
			else ui[i].setFillColor(sf::Color::White);
			window.draw(ui[i]);
		}
		if (typing)
		{
			enter.setPosition(window.getSize().x / 4 * 3, window.getSize().y - enter.getGlobalBounds().height - 10);
			enter.setFillColor(sf::Color::White);
			window.draw(enter);
		}
		window.display();
	}
    return 0;
}