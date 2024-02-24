#include "Engine/hWindow.hpp"
#include "Engine/hInput.hpp"
#include "Engine/hGlobal.hpp"

#define PUGIXML_WCHAR_MODE
#include <pugixml.hpp>

#include <iostream>
#include <filesystem>

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
	struct Level
	{
		Map map;
		sf::String name;
		sf::String bgPath;
		sf::Texture bgTex;
		sf::Sprite bgSpr;
		Level() { reset(); }
		void reset()
		{
			bgPath = "";
			bgTex = sf::Texture();
			bgSpr = sf::Sprite();
			map.reset();
			name = "";
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

//UI_WORLDINFO buttons
#define BTN_OPENWORLD 0
#define BTN_NEWLEVEL 1
#define BTN_DELETELEVEL 2
#define BTN_SAVEWORLD 3
#define BTN_CHOOSELEVELID 5
#define BTN_SETLEVELNAME 6

//UI_LEVELINFO buttons
#define BTN_RENAMELEVEL 0
#define BTN_SETBGPATH 1
#define BTN_RESIZEMAP 2
#define BTN_SETTILETEX 3
#define BTN_RESIZETILE 4
#define BTN_SCALEMAP 5

sf::Font font;

sf::Text input;
bool enter = false;

std::vector<sf::Text> ui;

int currentMenu = 0, currentInput = -1;

void updateUI()
{
	if (currentMenu == UI_WORLDINFO)
	{
		ui.resize(7);
		ui[0] = sf::Text(sf::String("Current file:\n") + World::file, font, 20);
		ui[1] = sf::Text(sf::String("New level"), font, 20);
		ui[2] = sf::Text(sf::String("Delete level"), font, 20);
		ui[3] = sf::Text(sf::String("Save world"), font, 20);
		if (!World::lvls.size()) return;
		ui[4] = sf::Text(sf::String("Level count: ") + std::to_string(World::lvls.size()), font, 20);
		ui[5] = sf::Text(sf::String("Current level ID: ") + std::to_string(World::currentLevel), font, 20);
		ui[6] = sf::Text(sf::String("Current level name: ") + World::lvls[World::currentLevel].name, font, 20);
	}
	else if (currentMenu == UI_LEVELINFO)
	{
		if (!World::lvls.size()) currentMenu = UI_WORLDINFO;
		auto *lvl = &World::lvls[World::currentLevel];
		ui.resize(6);
		ui[0] = sf::Text(sf::String("Level name: ") + lvl->name, font, 20);
		ui[1] = sf::Text(sf::String("Background file:\n") + lvl->bgPath, font, 20);
		ui[2] = sf::Text(sf::String("Map size: ") + std::to_string(lvl->map.mapSize.x) + " " + std::to_string(lvl->map.mapSize.y), font, 20);
		ui[3] = sf::Text(sf::String("Tile file:\n") + lvl->map.texPath, font, 20);
		ui[4] = sf::Text(sf::String("Tile size: ") + std::to_string(lvl->map.tileSize.x) + " " + std::to_string(lvl->map.tileSize.y), font, 20);
		ui[5] = sf::Text(sf::String("Map scale: ") + std::to_string(lvl->map.scale), font, 20);
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
		case BTN_OPENWORLD:
			World::loadFromFile(cmd);
			break;
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
		case BTN_SAVEWORLD:
			World::save(cmd);
			break;
		case BTN_CHOOSELEVELID:
			World::currentLevel = std::clamp(std::stoi(cmd.toAnsiString()), 0, (int)World::lvls.size() - 1);
			break;
		case BTN_SETLEVELNAME:
			World::lvls[World::currentLevel].name = cmd;
			break;
		default: break;
		}
	}
	else if (currentMenu == UI_LEVELINFO)
	{
		switch (currentInput)
		{
		case BTN_RENAMELEVEL:
			World::lvls[World::currentLevel].name = cmd;
			break;
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
		case BTN_SCALEMAP:
			World::lvls[World::currentLevel].map.scale = std::stof(cmd.toAnsiString());
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
		if (!enter && Input::isKeyPressed(sf::Keyboard::A)) { cam.move(-speed * Window::getDeltaTime(), 0); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::D)) { cam.move(speed * Window::getDeltaTime(), 0); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::W)) { cam.move(0, -speed * Window::getDeltaTime()); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::S)) { cam.move(0, speed * Window::getDeltaTime()); }
		if (!enter && Input::isKeyPressed(sf::Keyboard::Q))
		{
			if (lvl && currentMenu == UI_LEVELINFO) lvl->map.resize(abs(mPos.x), abs(mPos.y));
		}
		if (Input::isMBPressed(sf::Mouse::Left))
		{
			if (lvl && currentMenu == UI_LEVELINFO && Input::getMousePos().x <= uiLeft)
			if (mPos.x == std::clamp(mPos.x, 0, lvl->map.mapSize.x - 1) &&
				mPos.y == std::clamp(mPos.y, 0, lvl->map.mapSize.y - 1)) lvl->map.tiles[mPos.x][mPos.y] = currentTile;
		}
		else if (Input::isMBPressed(sf::Mouse::Right))
		{
			if (lvl && currentMenu == UI_LEVELINFO && Input::getMousePos().x <= uiLeft)
			if (mPos.x == std::clamp(mPos.x, 0, lvl->map.mapSize.x - 1) &&
				mPos.y == std::clamp(mPos.y, 0, lvl->map.mapSize.y - 1)) currentTile = lvl->map.tiles[mPos.x][mPos.y];
		}
		if (!enter && Input::isKeyJustPressed(sf::Keyboard::Num1)) { currentMenu = UI_WORLDINFO; }
		if (!enter && Input::isKeyJustPressed(sf::Keyboard::Num2)) { currentMenu = UI_LEVELINFO; }
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