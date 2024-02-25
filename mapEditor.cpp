#include "Engine/hWindow.hpp"
#include "Engine/hInput.hpp"
#include "Engine/hGlobal.hpp"
#include "Engine/hAnimation.hpp"
#include "Engine/hAssets.hpp"

#include <pugixml.hpp>

#include <iostream>
#include <filesystem>
#include <fstream>

class World
{
public:
	struct Map
	{
		sf::Vector2i mapSize;
		std::vector<std::vector<sf::Uint16>> tiles;
		sf::Texture tileTex;
		sf::String texPath;
		sf::Sprite tile;
		std::vector<sf::IntRect> tileRects;
		float scale;
		sf::Vector2i tileSize;
		Map() { reset(); }
		void reset()
		{
			mapSize = {0, 0};
			tiles.clear();
			tileTex = sf::Texture();
			texPath = "";
			tile = sf::Sprite();
			tileRects.clear();
			scale = 1;
			tileSize = {0, 0};
		}
		void draw(sf::RenderTarget *target)
		{
			tile.setTexture(tileTex);
			tile.setScale(scale, scale);
			for (int x = 0; x < mapSize.x; x++)
			{
				for (int y = 0; y < mapSize.y; y++)
				{
					if (tiles[x][y] == 0) { continue; }
					tile.setTextureRect(tileRects[tiles[x][y] - 1]);
					tile.setPosition(x * tileSize.x * scale, y * tileSize.y * scale);
					target->draw(tile);
				}
			}
		}
		void resize(int x, int y)
		{
			tiles.resize(x);
			for (int i = 0; i < x; i++)
			{
				tiles[i].resize(y);
			}
			mapSize = sf::Vector2i(x, y);
		}
		sf::Vector2f getPixelSize()
		{
			return sf::Vector2f(
				mapSize.x * tileSize.x * scale,
				mapSize.y * tileSize.y * scale
			);
		}
		void computeRects()
		{
			tileRects.clear();
			const auto texX = tileTex.getSize().x / tileSize.x,
				texY = tileTex.getSize().y / tileSize.y;
			for (short id = 0; id < texX * texY; id++)
			{
				sf::IntRect r;
				r.left = (int)(id % texX) * tileSize.x;
				r.top = (int)(id / texY) * tileSize.y;
				r.width = tileSize.x;
				r.height = tileSize.y;
				tileRects.push_back(r);
			}
		}
	};
	struct Camera
	{
		sf::View view;
		sf::String owner;
		sf::Vector2f offset;
		Camera()
		{
			view = sf::View({0, 0, 0, 0});
			owner = "";
			offset = {0, 0};
		}
		Camera(sf::Vector2f size, sf::Vector2f os, sf::String name)
		{
			view = sf::View({0, 0, size.x, size.y});
			offset = os;
			owner = name;
		}
		void update(sf::Vector2f mapSize)
		{
			auto *e = World::lvls[World::currentLevel].getSpawner(owner);
			sf::Vector2f pos;
			if (e == nullptr)
			{
				pos = offset;
			}
			else
			{
				pos = e->pos + offset;
			}
			sf::Vector2f min = view.getSize() / 2.0f;
			sf::Vector2f max = mapSize - (view.getSize() / 2.0f);
			if (view.getSize().x > mapSize.x) { min.x = max.x = mapSize.x / 2; }
			if (view.getSize().y > mapSize.y) { min.y = max.y = mapSize.y / 2; }
			view.setCenter(pos);
			view.setSize(abs(view.getSize().x), abs(view.getSize().y));
		}
	};
	struct Spawner
	{
		sf::Vector2f pos;
		sf::String entName;
		sf::Texture tex;
		sf::Sprite spr;
		Spawner() { pos = {0, 0}; entName = ""; }
		Spawner(sf::Vector2f xy, sf::String name)
		{
			pos = xy;
			entName = name;
			for (auto path : std::filesystem::recursive_directory_iterator("res/ents"))
			{
				std::ifstream ent(path.path().string());
				std::string skeletonFile;
				int bone = -1;
				while (!ent.eof())
				{
					std::string line;
					std::getline(ent, line);
					auto args = tr::splitStr(line, " ");
					if (args[0] == "Skeleton") { skeletonFile = args[1]; }
					if (args[0] == "gameName")
					{
						if (args[2] != entName) { skeletonFile = ""; bone = -1; break; }
					}
					if (args[0] == "headBone") { bone = std::stoi(args[2].toAnsiString()); }
				}
				if (!skeletonFile.empty() && bone != -1)
				{
					Skeleton s; s.loadFromFile(skeletonFile);
					s.update();
					auto *b = s.getBone(bone);
					tex.loadFromImage(b->tex->copyToImage());
					spr.setTexture(tex);
					spr.setTextureRect(b->spr.getTextureRect());
					spr.setOrigin(spr.getTextureRect().width / 2, spr.getTextureRect().height / 2);
					return;
				}
			}
		}
	};
	struct Level
	{
		Map map;
		sf::String name;
		sf::String bgPath;
		sf::Texture bgTex;
		sf::Sprite bgSpr;
		sf::String musicFile;
		Camera cam;
		std::vector<Spawner> spawns;
		Level() { reset(); }
		void reset()
		{
			bgPath = "";
			bgTex = sf::Texture();
			bgSpr = sf::Sprite();
			map.reset();
			name = musicFile = "";
			cam = Camera();
			spawns.clear();
		}
		void draw(sf::RenderTarget *target)
		{
			bgSpr.setTexture(bgTex, true);
			auto size = map.getPixelSize();
			bgSpr.setScale(
				size.x / bgTex.getSize().x,
				size.y / bgTex.getSize().y
			);
			target->draw(bgSpr);
			map.draw(target);
			for (int i = 0; i < spawns.size(); i++)
			{
				auto *s = &spawns[i];
				s->spr.setTexture(s->tex);
				s->spr.setPosition(s->pos);
				target->draw(s->spr);
			}
			cam.update(map.getPixelSize());
			sf::RectangleShape camRect(cam.view.getSize());
			camRect.setOrigin(camRect.getSize() / 2.0f);
			camRect.setPosition(cam.view.getCenter());
			camRect.setFillColor({0, 255, 0, 32});
			target->draw(camRect);
		}
		Spawner *getSpawner(sf::String name)
		{
			for (int i = 0; i < spawns.size(); i++) { if (spawns[i].entName == name) { return &spawns[i]; } }
			return nullptr;
		}
	};
	static std::vector<Level> lvls;
	static int currentLevel;
	static std::string file;
	static void init()
	{
		file = "None";
		lvls.clear();
		currentLevel = 0;
	}
	static void loadFromFile(std::string filename)
	{
		if (filename == "") { return; }
		init();
		if (!tr::strContains(filename, ".trworld")) { filename += ".trworld"; }
		if (!std::filesystem::exists(filename)) { file = "None"; return; }
		file = filename;
		pugi::xml_document doc;
		doc.load_file(pugi::as_wide(filename).c_str());
		for (auto lvl : doc.children())
		{
			World::Level level;
			level.name = lvl.attribute(L"name").as_string();
			auto map = lvl.child(L"map");
			level.map.texPath = map.attribute(L"tex").as_string();
			level.map.tileTex.loadFromFile(level.map.texPath);
			auto ts = tr::splitStr(map.attribute(L"tileSize").as_string(), " ");
			level.map.tileSize = {std::stoi(ts[0].toAnsiString()), std::stoi(ts[1].toAnsiString())};
			level.map.computeRects();
			auto ms = tr::splitStr(map.attribute(L"size").as_string(), " ");
			level.map.resize(std::stoi(ms[0].toAnsiString()), std::stoi(ms[1].toAnsiString()));
			auto tilemap = tr::splitStr(map.text().as_string(), "x");
			for (int y = 0; y < level.map.mapSize.y; y++)
			{
				for (int x = 0; x < level.map.mapSize.x; x++)
				{
					level.map.tiles[x][y] = std::stoi(tilemap[y * level.map.mapSize.x + x].toAnsiString());
				}
			}
			level.bgPath = lvl.child(L"background").attribute(L"path").as_string();
			level.bgTex.loadFromFile(level.bgPath);
			level.musicFile = lvl.child(L"music").attribute(L"path").as_string();
			auto camSize = tr::splitStr(lvl.child(L"camera").attribute(L"size").as_string(), " ");
			auto camOS = tr::splitStr(lvl.child(L"camera").attribute(L"offset").as_string(), " ");
			level.cam = Camera(
				{std::stof(camSize[0].toAnsiString()), std::stof(camSize[1].toAnsiString())},
				{std::stof(camOS[0].toAnsiString()), std::stof(camOS[1].toAnsiString())},
				lvl.child(L"camera").attribute(L"owner").as_string()
			);
			for (auto ent : lvl.children(L"entity"))
			{
				auto pos = tr::splitStr(ent.attribute(L"pos").as_string(), " ");
				level.spawns.push_back(Spawner(
					{std::stof(pos[0].toAnsiString()), std::stof(pos[1].toAnsiString())},
					ent.attribute(L"name").as_string()
				));
			}
			World::lvls.push_back(level);
		}
	}
	static void save(std::string filename)
	{
		if (!tr::strContains(filename, ".trworld")) { filename += ".trworld"; }
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
			map.append_attribute(L"tex") = pugi::as_wide(m->texPath).c_str();
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
			level.append_child(L"background").append_attribute(L"path") = lvls[i].bgPath.toWideString().c_str();
			//Music node
			level.append_child(L"music").append_attribute(L"path") = lvls[i].musicFile.toWideString().c_str();
			//Camera node
			auto cam = level.append_child(L"camera");
			cam.append_attribute(L"size") = pugi::as_wide(
				std::to_string(lvls[i].cam.view.getSize().x) + " " +
				std::to_string(lvls[i].cam.view.getSize().y)
			).c_str();
			cam.append_attribute(L"offset") = pugi::as_wide(
				std::to_string(lvls[i].cam.offset.x) + " " +
				std::to_string(lvls[i].cam.offset.y)
			).c_str();
			cam.append_attribute(L"owner") = lvls[i].cam.owner.toWideString().c_str();
			//Spawners
			for (int j = 0; j < lvls[i].spawns.size(); j++)
			{
				auto *s = &lvls[i].spawns[j];
				auto spawner = level.append_child(L"entity");
				spawner.append_attribute(L"name") = s->entName.toWideString().c_str();
				spawner.append_attribute(L"pos") = pugi::as_wide(
					std::to_string(s->pos.x) + " " +
					std::to_string(s->pos.y)
				).c_str();
			}
		}
		doc.save_file(pugi::as_wide(filename).c_str());
	}
	static void draw(sf::RenderTarget *target)
	{
		if (!lvls.size()) { return; }
		lvls[currentLevel].draw(target);
		sf::RectangleShape rect(lvls[currentLevel].map.getPixelSize());
		rect.setFillColor({0, 0, 0, 0});
		rect.setOutlineColor(sf::Color::Red);
		rect.setOutlineThickness(2);
		target->draw(rect);
	}
};

std::vector<World::Level> World::lvls;
int World::currentLevel;
std::string World::file;

#define UI_WORLDINFO 0
#define UI_LEVELINFO 1
#define UI_VISUALS 2
#define UI_SPAWNERS 3

//UI_WORLDINFO buttons
#define BTN_OPENWORLD 0
#define BTN_NEWLEVEL 1
#define BTN_DELETELEVEL 2
#define BTN_SAVEWORLD 3
#define BTN_CHOOSELEVELID 5
#define BTN_SETLEVELNAME 6
#define BTN_SETMUSICFILE 7

//UI_LEVELINFO buttons
#define BTN_RENAMELEVEL 0
#define BTN_SETBGPATH 1
#define BTN_RESIZEMAP 2
#define BTN_SETTILETEX 3
#define BTN_RESIZETILE 4
#define BTN_SCALEMAP 5

//UI_VISUALS buttons
#define BTN_MOVECAM 0
#define BTN_RESIZECAM 1
#define BTN_SETCAMOWNER 2

//UI_SPAWNERS buttons
#define BTN_NEWSPAWNER 1
#define BTN_DELETESPAWNER 2
#define BTN_RENAMESPAWNER 3
#define BTN_MOVESPAWNER 4

sf::Font font;

sf::Text input;
bool enter = false;

std::vector<sf::Text> ui;

int currentMenu = 0, currentInput = -1, currentSpawner = -1;

void updateUI()
{
	if (currentMenu == UI_WORLDINFO)
	{
		currentSpawner = -1;
		ui.resize(4);
		ui[0] = sf::Text(sf::String("Current file:\n") + World::file, font, 20);
		ui[1] = sf::Text(sf::String("New level"), font, 20);
		ui[2] = sf::Text(sf::String("Delete level"), font, 20);
		ui[3] = sf::Text(sf::String("Save world"), font, 20);
		if (!World::lvls.size()) return;
		ui.resize(8);
		ui[4] = sf::Text(sf::String("Level count: ") + std::to_string(World::lvls.size()), font, 20);
		ui[5] = sf::Text(sf::String("Current level ID: ") + std::to_string(World::currentLevel), font, 20);
		ui[6] = sf::Text(sf::String("Current level name: ") + World::lvls[World::currentLevel].name, font, 20);
		ui[7] = sf::Text(sf::String("Music file:\n") + World::lvls[World::currentLevel].musicFile, font, 20);
	}
	else if (currentMenu == UI_LEVELINFO)
	{
		currentSpawner = -1;
		if (!World::lvls.size()) { currentMenu = UI_WORLDINFO; return; }
		auto *lvl = &World::lvls[World::currentLevel];
		ui.resize(7);
		ui[0] = sf::Text(sf::String("Level name: ") + lvl->name, font, 20);
		ui[1] = sf::Text(sf::String("Background file:\n") + lvl->bgPath, font, 20);
		ui[2] = sf::Text(sf::String("Map size: ") + std::to_string(lvl->map.mapSize.x) + " " + std::to_string(lvl->map.mapSize.y), font, 20);
		ui[3] = sf::Text(sf::String("Tile file:\n") + lvl->map.texPath, font, 20);
		ui[4] = sf::Text(sf::String("Tile size: ") + std::to_string(lvl->map.tileSize.x) + " " + std::to_string(lvl->map.tileSize.y), font, 20);
		ui[5] = sf::Text(sf::String("Map scale: ") + std::to_string(lvl->map.scale), font, 20);
		ui[6] = sf::Text(sf::String("Map pixel size:\n") +
			std::to_string(lvl->map.getPixelSize().x) + "\n" + std::to_string(lvl->map.getPixelSize().y), font, 20);
	}
	else if (currentMenu == UI_VISUALS)
	{
		currentSpawner = -1;
		if (!World::lvls.size()) { currentMenu = UI_WORLDINFO; return; }
		auto *lvl = &World::lvls[World::currentLevel];
		ui.resize(3);
		ui[0] = sf::Text(sf::String("Camera offset:\n") + std::to_string(lvl->cam.offset.x) + "\n" + std::to_string(lvl->cam.offset.y), font, 20);
		ui[1] = sf::Text(sf::String("Camera size:\n") +
			std::to_string(lvl->cam.view.getSize().x) + "\n" + std::to_string(lvl->cam.view.getSize().y), font, 20);
		ui[2] = sf::Text(sf::String("Camera owner name:\n") + lvl->cam.owner, font, 20);
	}
	else if (currentMenu == UI_SPAWNERS)
	{
		if (!World::lvls.size()) { currentMenu = UI_WORLDINFO; return; }
		auto *lvl = &World::lvls[World::currentLevel];
		ui.resize(3);
		ui[0] = sf::Text(sf::String("Spawners count: ") + std::to_string(lvl->spawns.size()), font, 20);
		ui[1] = sf::Text(sf::String("New spawner"), font, 20);
		ui[2] = sf::Text(sf::String("Delete spawner"), font, 20);
		if (currentSpawner == -1) return;
		ui.resize(5);
		auto *s = &lvl->spawns[currentSpawner];
		ui[3] = sf::Text(sf::String("Spawner name: ") + s->entName, font, 20);
		ui[4] = sf::Text(sf::String("Spawner position:\n") + std::to_string(s->pos.x) + "\n" + std::to_string(s->pos.y), font, 20);
	}
}

void setInput(int buttonID)
{
	enter = true;
	currentInput = buttonID;
	input.setString(">|");
}

void execute()
{
	if (currentInput == -1) { return; }
	auto cmd = input.getString();
	cmd = cmd.substring(1, cmd.getSize() - 2);
	if (currentMenu == UI_WORLDINFO)
	{
		switch (currentInput)
		{
		case BTN_OPENWORLD: World::loadFromFile(cmd); break;
		case BTN_NEWLEVEL:
			World::lvls.push_back(World::Level());
			World::currentLevel = World::lvls.size() - 1;
			World::lvls[World::currentLevel].name = cmd;
			break;
		case BTN_DELETELEVEL:
			for (int i = 0; i < World::lvls.size(); i++)
			{
				if (World::lvls[i].name == cmd)
				{
					World::lvls.erase(World::lvls.begin() + i);
					if (World::currentLevel == World::lvls.size()) World::currentLevel--;
				}
			}
		case BTN_SAVEWORLD: World::save(cmd); break;
		case BTN_CHOOSELEVELID: World::currentLevel = std::clamp(std::stoi(cmd.toAnsiString()), 0, (int)World::lvls.size() - 1); break;
		case BTN_SETLEVELNAME: World::lvls[World::currentLevel].name = cmd; break;
		case BTN_SETMUSICFILE: World::lvls[World::currentLevel].musicFile = cmd; break;
		default: break;
		}
	}
	else if (currentMenu == UI_LEVELINFO)
	{
		switch (currentInput)
		{
		case BTN_RENAMELEVEL: World::lvls[World::currentLevel].name = cmd; break;
		case BTN_SETBGPATH:
			World::lvls[World::currentLevel].bgPath = cmd;
			World::lvls[World::currentLevel].bgTex.loadFromFile(World::lvls[World::currentLevel].bgPath);
			break;
		case BTN_RESIZEMAP:
			World::lvls[World::currentLevel].map.resize(
				std::stoi(tr::splitStr(cmd, " ")[0].toAnsiString()),
				std::stoi(tr::splitStr(cmd, " ")[1].toAnsiString())
			);
		case BTN_SETTILETEX:
			World::lvls[World::currentLevel].map.texPath = cmd;
			World::lvls[World::currentLevel].map.tileTex.loadFromFile(World::lvls[World::currentLevel].map.texPath);
			World::lvls[World::currentLevel].map.computeRects();
			break;
		case BTN_RESIZETILE:
			World::lvls[World::currentLevel].map.tileSize = {
				std::stoi(tr::splitStr(cmd, " ")[0].toAnsiString()),
				std::stoi(tr::splitStr(cmd, " ")[1].toAnsiString())
			};
			World::lvls[World::currentLevel].map.computeRects();
			break;
		case BTN_SCALEMAP: World::lvls[World::currentLevel].map.scale = std::stof(cmd.toAnsiString()); break;
		default:
			break;
		}
	}
	else if (currentMenu == UI_VISUALS)
	{
		switch (currentInput)
		{
		case BTN_MOVECAM:
			World::lvls[World::currentLevel].cam.offset = {
				std::stof(tr::splitStr(cmd, " ")[0].toAnsiString()),
				std::stof(tr::splitStr(cmd, " ")[1].toAnsiString())
			};
			break;
		case BTN_RESIZECAM:
			World::lvls[World::currentLevel].cam.view.setSize({
				std::stof(tr::splitStr(cmd, " ")[0].toAnsiString()),
				std::stof(tr::splitStr(cmd, " ")[1].toAnsiString())
			});
			break;
		case BTN_SETCAMOWNER:
			World::lvls[World::currentLevel].cam.owner = cmd;
		default:
			break;
		}
	}
	else if (currentMenu == UI_SPAWNERS)
	{
		switch (currentInput)
		{
		case BTN_NEWSPAWNER:
			World::lvls[World::currentLevel].spawns.push_back({{0, 0}, cmd});
			break;
		case BTN_DELETESPAWNER:
			for (int i = 0; i < World::lvls[World::currentLevel].spawns.size(); i++)
			{
				if (World::lvls[World::currentLevel].spawns[i].entName == cmd)
				{
					World::lvls[World::currentLevel].spawns.erase(World::lvls[World::currentLevel].spawns.begin() + i);
					if (currentSpawner == World::lvls[World::currentLevel].spawns.size()) currentSpawner--;
				}
			}
			break;
		case BTN_RENAMESPAWNER:
			World::lvls[World::currentLevel].spawns[currentSpawner] = World::Spawner(
				{
					World::lvls[World::currentLevel].spawns[currentSpawner].pos.x,
					World::lvls[World::currentLevel].spawns[currentSpawner].pos.y
				},
				cmd
			);
			break;
		case BTN_MOVESPAWNER:
			World::lvls[World::currentLevel].spawns[currentSpawner].pos = {
				std::stof(tr::splitStr(cmd, " ")[0].toAnsiString()),
				std::stof(tr::splitStr(cmd, " ")[1].toAnsiString())
			};
		default:
			break;
		}
	}
	input.setString("");
	enter = false;
	currentInput = -1;
}

int main(int argc, char* argv[])
{
	AssetManager::init();
	Window::init(argc, argv);
	Window::setTitle("MapEditor");
	auto uiLeft = Window::getSize().x / 4 * 3;
	auto *render = Window::getScreen();
	render->create(uiLeft, Window::getSize().y);

	font.loadFromFile("res/global/font.ttf");

	input = sf::Text("", font, 20);
	input.setOrigin(0, 30);
	input.setFillColor(sf::Color::White);

	Window::resetTime();

	World::init();

	auto cam = Window::getScreen()->getView();

	sf::RectangleShape tile;
	tile.setFillColor({0, 0, 0, 0});
	tile.setOutlineColor(sf::Color::Blue);
	tile.setOutlineThickness(-2);
	sf::Sprite preview;

	int currentTile = 0;

	while (Window::isOpen())
	{
		Window::update();
		if (Window::hasEvent(sf::Event::Resized))
		{
			auto e = Window::getEvent(sf::Event::Resized);
			uiLeft = e.size.width / 4 * 3;
			render->create(uiLeft, e.size.height);
		}
		if (Window::hasEvent(sf::Event::TextEntered) && enter)
		{
			auto ch = Window::getEvent(sf::Event::TextEntered).text.unicode;
			auto str = input.getString();
			if (ch == 8) { if (str.getSize() > 2) str.erase(str.getSize() - 2); }
			else if (ch == 13) { execute(); str = ""; }
			else str.insert(str.getSize() - 1, sf::String(ch));
			input.setString(str);
		}

		World::Level *lvl = nullptr;
		if (World::lvls.size()) { lvl = &World::lvls[World::currentLevel]; }
		if (Window::hasEvent(sf::Event::MouseWheelMoved) && lvl)
		{
			auto e = Window::getEvent(sf::Event::MouseWheelMoved).mouseWheel;
			currentTile -= e.delta;
			if (currentTile < 0) currentTile = lvl->map.tileRects.size() - 1;
			else if (currentTile >= lvl->map.tileRects.size()) currentTile = 0;
		}
		sf::Vector2i mPos;
		if (lvl) mPos = {
			int(Input::getMousePos(true).x / (lvl->map.tileSize.x * lvl->map.scale)),
			int(Input::getMousePos(true).y / (lvl->map.tileSize.y * lvl->map.scale))
		};

		float speed = 200;
		if (!enter && Input::isKeyPressed(sf::Keyboard::LShift)) speed *= 2;
		if (!enter && Input::isKeyPressed(sf::Keyboard::A) && currentMenu == UI_WORLDINFO) { cam.move(-speed * Window::getDeltaTime(), 0); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::D) && currentMenu == UI_WORLDINFO) { cam.move(speed * Window::getDeltaTime(), 0); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::W) && currentMenu == UI_WORLDINFO) { cam.move(0, -speed * Window::getDeltaTime()); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::S) && currentMenu == UI_WORLDINFO) { cam.move(0, speed * Window::getDeltaTime()); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::Q) && lvl)
		{
			auto mouse = (sf::Vector2f)((sf::Vector2i)Input::getMousePos(true));
			if (currentMenu == UI_LEVELINFO) lvl->map.resize(abs(mPos.x), abs(mPos.y));
			else if (currentMenu == UI_VISUALS) lvl->cam.offset = mouse;
			else if (currentMenu == UI_SPAWNERS && currentSpawner != -1) lvl->spawns[currentSpawner].pos = mouse;
		}
		if (!enter && Input::isKeyPressed(sf::Keyboard::W) && lvl)
		{
			if (currentMenu == UI_VISUALS)
				lvl->cam.view.setSize((sf::Vector2f)((sf::Vector2i)Input::getMousePos(true)) - lvl->cam.view.getCenter() + lvl->cam.view.getSize() / 2.0f);
		}
		if (Input::isMBPressed(sf::Mouse::Left) && currentMenu == UI_WORLDINFO)
		{
			if (lvl && currentMenu == UI_LEVELINFO && Input::getMousePos().x <= uiLeft)
			if (mPos.x == std::clamp(mPos.x, 0, lvl->map.mapSize.x - 1) &&
				mPos.y == std::clamp(mPos.y, 0, lvl->map.mapSize.y - 1)) lvl->map.tiles[mPos.x][mPos.y] = currentTile;
		}
		else if (Input::isMBPressed(sf::Mouse::Right) && currentMenu == UI_WORLDINFO)
		{
			if (lvl && currentMenu == UI_LEVELINFO && Input::getMousePos().x <= uiLeft)
			if (mPos.x == std::clamp(mPos.x, 0, lvl->map.mapSize.x - 1) &&
				mPos.y == std::clamp(mPos.y, 0, lvl->map.mapSize.y - 1)) currentTile = lvl->map.tiles[mPos.x][mPos.y];
		}
		if (currentMenu == UI_SPAWNERS && lvl)
		{
			if (Input::isMBJustPressed(sf::Mouse::Left) && Input::getMousePos().x <= uiLeft) { currentSpawner = -1; }
			for (int i = 0; i < lvl->spawns.size(); i++)
			{
				if (lvl->spawns[i].spr.getGlobalBounds().contains(Input::getMousePos(true)))
				{
					lvl->spawns[i].spr.setColor(sf::Color::White);
					if (Input::isMBJustPressed(sf::Mouse::Left) && Input::getMousePos().x <= uiLeft) { currentSpawner = i; }
				}
				else lvl->spawns[i].spr.setColor({255, 255, 255, 127});
			}
			if (currentSpawner != -1) lvl->spawns[currentSpawner].spr.setColor(sf::Color::White);
		}
		if (!enter && Input::isKeyJustPressed(sf::Keyboard::Num1)) { currentMenu = UI_WORLDINFO; }
		if (!enter && Input::isKeyJustPressed(sf::Keyboard::Num2)) { currentMenu = UI_LEVELINFO; }
		if (!enter && Input::isKeyJustPressed(sf::Keyboard::Num3)) { currentMenu = UI_VISUALS; }
		if (!enter && Input::isKeyJustPressed(sf::Keyboard::Num4)) { currentMenu = UI_SPAWNERS; }
		if (Input::isKeyJustPressed(sf::Keyboard::Enter)) { enter = false; if (!input.getString().isEmpty()) { execute(); } }

		updateUI();

		Window::getScreen()->setView(cam);

		Window::clear();
		render->clear({127, 127, 127, 255});
		World::draw(Window::getScreen());
		if (lvl)
		{
			sf::Vector2f tileSize = {
				lvl->map.tileSize.x * lvl->map.scale,
				lvl->map.tileSize.y * lvl->map.scale
			};
			preview.setTexture(lvl->map.tileTex);
			preview.setTextureRect(currentTile ? lvl->map.tileRects[currentTile - 1] : sf::IntRect(0, 0, 0, 0));
			preview.setPosition(mPos.x * tileSize.x, mPos.y * tileSize.y);
			preview.setScale(lvl->map.scale, lvl->map.scale);
			Window::draw(preview, false);
			tile.setPosition(preview.getPosition());
			tile.setSize(tileSize);
			Window::draw(tile, false);
		}
		Window::drawScreen();
		for (int i = 0; i < ui.size(); i++)
		{
			auto *txt = &ui[i];
			txt->setFont(font);
			txt->setPosition(uiLeft, i > 0 ? ui[i - 1].getPosition().y + ui[i - 1].getGlobalBounds().height + 8 : 0);
			if (txt->getGlobalBounds().contains(Input::getMousePos()))
			{
				txt->setFillColor(sf::Color::Red);
				if (Input::isMBJustPressed(sf::Mouse::Left)) { setInput(i); }
			}
			else { txt->setFillColor(sf::Color::White); }
			Window::draw(*txt);
		}
		input.setFont(font);
		input.setPosition(uiLeft, Window::getSize().y);
		Window::draw(input);
		Window::display();
	}
	return 0;
}