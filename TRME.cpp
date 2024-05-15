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
int currentLevel, currentPage;
sf::Vector2f mouse;
sf::Text enter;
bool typing;
sf::String assets, currentPath;
sf::Vector2i command;

struct Level
{
	struct Map
	{
		sf::Texture tileTex;
		sf::Vector2i tileSize;
		std::vector<sf::IntRect> rects;
		std::vector<std::vector<sf::Uint16>> tiles;
		float scale;
		sf::Vector2i mapSize;
		Map()
		{
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
			tiles.resize(x, std::vector<sf::Uint16>(y));
			mapSize = {x, y};
		}
		void draw()
		{
			sf::Sprite tile(tileTex);
			tile.setScale(scale, scale);
			for (int x = 0; x < mapSize.x; x++)
			{
				for (int y = 0; y < mapSize.y; y++)
				{
					auto id = tiles[x][y];
					if (!id) continue;
					tile.setTextureRect(rects[id]);
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
				auto path = p.path().string().substr(currentPath.getSize());
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
	sf::String bgPath;
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
		level.map.tileTex.loadFromFile(pugi::as_utf8(map.attribute(L"tex").as_string()));
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

#define PageMain 0

//Main page
#define MainOpen 0
#define MainSave 1
#define MainName 2
#define MainLevels 3
#define MainBG 4
#define MainBounds 5
#define MainMusic 6
#define MainVolume 7
#define MainGravity 8
#define MainCamera 9

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
		while (tr::strContains(file, "\\")) file.replace("\\", "/");
		return file;
	}
	return "Fail"; 
}

std::vector<sf::Text> ui;

void reloadUI()
{
	ui.clear();
	if (currentPage == PageMain) { ui.resize(10, {"", font, 20}); }
}

void updateUI()
{
	if (currentPage == PageMain)
	{
		ui[0].setString("Open world");
		ui[1].setString("Save world");
		if (!lvls.size()) return;
		auto l = &lvls[currentLevel];
		ui[2].setString("Level name:\n" + l->name);
		ui[3].setString("Level count: " + std::to_string(lvls.size()));
		ui[4].setString("Background path:\n" + l->bgPath);
		ui[5].setString("Background bounds:\n" +
			std::to_string((int)l->bgBounds.left) + " " +
			std::to_string((int)l->bgBounds.top) + " " +
			std::to_string((int)l->bgBounds.width) + " " +
			std::to_string((int)l->bgBounds.height)
		);
		ui[6].setString("Music path:\n" + l->music);
		ui[7].setString("Music volume: " + std::to_string((int)l->musicVolume));
		ui[8].setString("Gravity: " +
			std::to_string(l->gravity.x) + " " +
			std::to_string(l->gravity.y)
		);
		ui[9].setString("Camera size/offset:\n" +
			std::to_string((int)l->camera.left) + " " +
			std::to_string((int)l->camera.top) + " " +
			std::to_string((int)l->camera.width) + " " +
			std::to_string((int)l->camera.height)
		);
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
		}
		if (btn == MainName)
		{
			typing = true;
			enter.setString("New trigger name:\n");
		}
		if (btn == MainLevels)
		{
			typing = true;
			enter.setString("Choose level by ID:\n");
		}
		if (btn == MainBG)
		{
			auto path = openWindow("Texture (*.png)\0*.png\0");
			if (path == "Fail") return;
			lvls[currentLevel].bgPath = path;
			lvls[currentLevel].bgTex.loadFromFile(lvls[currentLevel].bgPath);
		}
		if (btn == MainBounds)
		{
			typing = true;
			enter.setString("Set background bounds:\n");
		}
		if (btn == MainMusic)
		{
			auto path = openWindow("Music (*.ogg)\0*.ogg\0");
			if (path == "Fail") return;
			lvls[currentLevel].music = path;
		}
		if (btn == MainVolume)
		{
			typing = true;
			enter.setString("Set music volume:\n");
		}
		if (btn == MainGravity)
		{
			typing = true;
			enter.setString("Set level gravity:\n");
		}
		if (btn == MainCamera)
		{
			typing = true;
			enter.setString("Set new camera size and offset:\n");
		}
		if (typing) command = {page, btn};
	}
}

void cmd()
{
	// 
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
		}

		if (window.hasFocus())
		{
			float speed = 200;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { camera.move(-speed * deltaTime, 0); }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { camera.move(speed * deltaTime, 0); }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { camera.move(0, -speed * deltaTime); }
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { camera.move(0, speed * deltaTime); }
		}

		
		window.clear();
		screen.setView(camera);
		screen.clear({127, 127, 127});
		if (lvls.size())
		{
			auto *lvl = &lvls[currentLevel];
			lvl->draw();
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
		window.display();
	}
    return 0;
}