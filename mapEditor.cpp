#include "Engine/hWindow.hpp"
#include "Engine/hInput.hpp"
#include "Engine/hGlobal.hpp"

#define PUGIXML_WCHAR_MODE
#include <pugixml.hpp>

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
		void parse(pugi::xml_node node);
		void draw(sf::RenderTarget *target)
		{
			tile.setTexture(tileTex);
			tile.setScale(1, 1);
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
		Level() { reset(); }
		void reset()
		{
			map.reset();
			name = "";
		}
		void draw(sf::RenderTarget *target)
		{
			map.draw(target);
		}
	};
	static std::vector<Level> lvls;
	static int currentLevel;
	static void init()
	{
		lvls.clear();
		currentLevel = 0;
	}
	static void loadFromFile(std::string filename)
	{
		init();
		if (!tr::strContains(filename, ".trworld")) { filename += ".trworld"; }
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

#define UI_MAPINFO 0
#define UI_VISUALS 1

#define BTN_MAPSIZE 0

sf::Font font;

std::vector<sf::Text> ui;

int currentMenu = 0;

void updateUI()
{
	if (currentMenu == UI_MAPINFO)
	{
		ui.resize(1);
		ui[0] = sf::Text("Map size: ", font, 20);
	}
}

void execute(int buttonID)
{
	if (currentMenu == UI_MAPINFO)
	{
		switch (buttonID)
		{
		case BTN_MAPSIZE:
			return;
		default: return;
		}
	}
	else if (currentMenu == UI_VISUALS)
	{
		// 
	}
}

int main(int argc, char* argv[])
{
	Window::init(argc, argv);
	Window::setTitle("MapEditor");
	auto uiLeft = Window::getSize().x / 4 * 3;
	auto *render = Window::getScreen();
	render->create(uiLeft, Window::getSize().y);

	font.loadFromFile("res/global/font.ttf");

	Window::resetTime();

	World::loadFromFile("res/worlds/xml.trworld");

	while (Window::isOpen())
	{
		Window::update();
		if (Window::hasEvent(sf::Event::Resized))
		{
			auto e = Window::getEvent(sf::Event::Resized);
			uiLeft = e.size.width / 4 * 3;
			render->create(uiLeft, e.size.height);
		}

		if (Input::isKeyJustPressed(sf::Keyboard::Num1)) { currentMenu = UI_MAPINFO; }
		if (Input::isKeyJustPressed(sf::Keyboard::Num2)) { currentMenu = UI_VISUALS; }

		Window::clear();
		render->clear({127, 127, 127, 255});
		World::draw(Window::getScreen());
		Window::drawScreen();
		for (int i = 0; i < ui.size(); i++)
		{
			auto *txt = &ui[i];
			txt->setFont(font);
			txt->setPosition(uiLeft, i * 30);
			if (txt->getGlobalBounds().contains(Input::getMousePos()))
			{
				txt->setFillColor(sf::Color::Red);
				if (Input::isMBJustPressed(sf::Mouse::Left)) { execute(i); }
			}
			else { txt->setFillColor(sf::Color::White); }
			Window::draw(*txt);
		}
		Window::display();
	}
	return 0;
}