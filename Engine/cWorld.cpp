#include "hWorld.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hInput.hpp"
#include "hInventory.hpp"
#include "hDialogue.hpp"
#include "hUI.hpp"

#include <iostream>

#include <pugiconfig.hpp>

std::vector<Entity> World::ents;
sf::RenderTexture World::screen;
std::vector<World::Level> World::lvls;
int World::currentLevel;
b2World *World::world;
sf::Music World::music;
bool World::active;

void World::init()
{
	ents.clear();
	if (world != nullptr) delete world;
	world = new b2World({0, 0});
	for (auto file : AssetManager::getTexts(".trent"))
	{
		ents.push_back(Entity(file));
	}
	lvls.clear();
	currentLevel = 0;
	music.setRelativeToListener(true);
	music.stop();
	music.setVolume(Window::getVar("musicVolume"));
	active = false;
	
	Inventory::init();
}

void World::loadFromFile(std::string filename)
{
	init();
	pugi::xml_document doc;
	doc.load_file(pugi::as_wide(filename).c_str());
	for (auto lvl : doc.children())
	{
		World::Level level;
		level.name = lvl.attribute(L"name").as_string();
		auto map = lvl.child(L"map");
		level.map.tileTex = AssetManager::getTexture(pugi::as_utf8(map.attribute(L"tex").as_string()));
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
		level.bgTex = AssetManager::getTexture(pugi::as_utf8(lvl.child(L"background").attribute(L"path").as_string()));
		level.musicFilename = pugi::as_utf8(lvl.child(L"music").attribute(L"path").as_string());
		World::lvls.push_back(level);
	}
	active = true;
}

void World::update()
{
	if (currentLevel < 0 || currentLevel >= lvls.size()) return;
	if (!active)
	{
		music.pause();
		for (int i = 0; i < lvls[currentLevel].sounds.size(); i++)
		{
			lvls[currentLevel].sounds[i].sound.pause();
		}
		return;
	}
	lvls[currentLevel].update();
}

void World::draw()
{
	if (currentLevel < 0 || currentLevel >= lvls.size() || !active) return;
	lvls[currentLevel].draw(&screen);
}

sf::RenderTexture *World::getScreen() { return &screen; }

Entity *World::getEnt(sf::String name)
{
	for (int i = 0; i < ents.size(); i++)
	{
		if (ents[i].getVar("gameName") == name)
		{
			return &ents[i];
		}
	}
	return nullptr;
}

World::Map::Map() { reset(); }

void World::Map::draw(sf::RenderTarget *target)
{
	tile.setTexture(*tileTex);
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

void World::Map::resize(int x, int y)
{
	tiles.resize(x);
	for (int i = 0; i < x; i++)
	{
		tiles[i].resize(y);
	}
	mapSize = sf::Vector2i(x, y);
}

void World::Map::reset()
{
	tiles.clear();
	tileTex = nullptr;
	tile = sf::Sprite();
	tileRects.clear();
}

void World::Map::loadFromFile(std::string filename)
{
	auto file = tr::splitStr(AssetManager::getText(filename), "\n");
	for (int i = 0; i < file.size(); i++)
	{
		auto line = file[i];
		auto args = tr::splitStr(line, " ");
		if (args[0].find("#") != std::string::npos) { continue; }
		else if (args[0].find("MapSize") != std::string::npos)
		{
			mapSize = { std::stoi(args[1].toAnsiString()), std::stoi(args[2].toAnsiString()) };
			tiles.clear();
		}
		else if (args[0].find("TileTexture") != std::string::npos)
		{
			texFilename = args[1];
			tileTex = AssetManager::getTexture(texFilename);
		}
		else if (args[0].find("TileSize") != std::string::npos)
		{
			tileSize = { std::stoi(args[1].toAnsiString()), std::stoi(args[2].toAnsiString())};
			const auto texX = tileTex->getSize().x / tileSize.x,
				texY = tileTex->getSize().y / tileSize.y;
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
		else if (args[0].find("TileScale") != std::string::npos)
		{
			scale = std::stof(args[1].toAnsiString());
		}
		else if (args[0].find("StartMap") != std::string::npos)
		{
			args = tr::splitStr(file[++i], " ");
			std::vector<std::string> wld;
			while (args[0].find("EndMap") == std::string::npos)
			{
				wld.push_back(args[0]);
				args = tr::splitStr(file[++i], " ");
			}
			for (short x = 0; x < mapSize.x; x++)
			{
				std::vector<sf::Uint16> blocks;
				for (short y = 0; y < mapSize.y; y++)
				{
					auto b = tr::splitStr(wld[y], "x");
					blocks.push_back(std::stoi(b[x].toAnsiString()));
				}
				tiles.push_back(blocks);
			}
		}
	}
}

sf::Vector2f World::Map::getPixelSize()
{
	return sf::Vector2f(
		mapSize.x * tileSize.x * scale,
		mapSize.y * tileSize.y * scale
	);
}

World::Level::Level()
{
	reset();
}

World::Level::~Level()
{
	reset();
}

void World::Level::reset()
{
	map.reset();
	name = "";
	bgTex = nullptr;
	bgSpr = sf::Sprite();
	gravity = {0, 0};
	cam = Camera();
	triggers.clear();
	sounds.clear();
	lights.clear();
	items.clear();
	controls.clear();
	while (ents.size() > 0)
	{
		auto *e = ents[ents.size() - 1];
		if (e->getVar("Copyable")) { delete e; }
		ents.pop_back();
	}
}

Entity *World::Level::getEntity(sf::String name)
{
	for (int i = 0; i < ents.size(); i++)
	{
		if (ents[i]->getVar("gameName") == name) { return ents[i]; }
	}
	return nullptr;
}

void World::Level::update()
{
	world->SetGravity(gravity);
	world->Step(Window::getDeltaTime(), 12, 8);
	auto musicStatus = music.getStatus();
	if (musicStatus == sf::Music::Status::Stopped)
	{
		music.openFromFile(musicFilename);
		music.play();
	}
	else if (musicStatus == sf::Music::Status::Paused) { music.play(); }
	for (int i = 0; i < sounds.size(); i++)
	{
		auto *s = &sounds[i].sound;
		if (s->getStatus() == sf::Sound::Status::Paused) { s->play(); }
		if (s->getStatus() == sf::Sound::Status::Stopped && s->getPlayingOffset().asSeconds() > 0)
		{
			sounds.erase(sounds.begin() + i);
		}
	}
	for (int i = 0; i < controls.size(); i++)
	{
		auto *ctrl = Input::getControl(controls[i].ctrlID);
		auto *ent = getEntity(controls[i].entName);
		if (ctrl == nullptr || ent == nullptr) { continue; }
		for (int j = 0; j < ctrl->vars.size(); j++)
		{
			auto *v = &ctrl->vars[j];
			ent->setVar(v->name, v->value);
		}
	}
	for (int i = 0; i < ents.size(); i++)
	{
		ents[i]->update();
		if (!ents[i]->weapon.meleeOrRange && ents[i]->getVar("attacking"))
		{
			auto r = ents[i]->weapon.spr.getGlobalBounds();
			for (int j = 0; j < ents.size(); j++)
			{
				if (i != j &&
					r.intersects(ents[j]->getHitbox()) &&
					ents[j]->getVar("noHurtTimer") >= ents[j]->getVar("damageCD"))
				{
					for (int k = 0; k < ents[i]->weapon.effects.size(); k++)
					{
						ents[j]->addEffect(ents[i]->weapon.effects[k]);
					}
					ents[j]->setVar("noHurtTimer", 0);
				}
			}
		}
	}
	auto *camOwner = getEnt(cam.owner);
	if (camOwner)
	{
		auto pos = camOwner->getPosition();
		sf::Listener::setPosition(pos.x, 0, pos.y);
		if (camOwner->getVar("interacting"))
		{
			for (int i = 0; i < triggers.size(); i++)
			{
				if (camOwner->getHitbox().intersects(triggers[i].rect))
				{
					auto prompt = triggers[i].getVar("cmd").str;
					if (prompt.isEmpty()) { continue; }
					auto cmd = tr::splitStr(prompt, "|");
					for (int j = 0; j < cmd.size(); j++)
					{
						tr::execute(cmd[j]);
					}
				}
			}
		}
		camOwner->setVar("interacting", 0);
		for (int j = 0; j < items.size(); j++)
		{
			for (int i = 0; i < items.size(); i++)
			{
				if (j != i &&
					items[j].item.item.spr.getGlobalBounds().intersects(items[i].item.item.spr.getGlobalBounds()) &&
					items[i].cooldown.getElapsedTime().asSeconds() >= 1.5f &&
					items[i].item.item.id == items[j].item.item.id &&
					items[i].item.item.type == items[j].item.item.type)
				{
					items[i].rb.destroy();
					items[j].item.count += items[i].item.count;
					items.erase(items.begin() + i--);
				}
			}
			if (camOwner->getHitbox().intersects(items[j].item.item.spr.getGlobalBounds()) && items[j].cooldown.getElapsedTime().asSeconds() >= 1.5f)
			{
				Inventory::addItem(items[j].item.item.type, items[j].item.item.id, items[j].item.count);
				items[j].rb.destroy();
				items.erase(items.begin() + j--);
			}
		}
	}
}

void World::Level::draw(sf::RenderTarget *target)
{
	auto mapSize = map.getPixelSize();
	if (screen.getSize() != (sf::Vector2u)mapSize) { screen.create(mapSize.x, mapSize.y); }
	screen.clear();
	if (bgTex != nullptr) bgSpr.setTexture(*bgTex);
	float bgScale = (mapSize.y - map.tileSize.y * map.scale) / bgSpr.getTextureRect().height;
	bgSpr.setScale(bgScale, bgScale);
	target->draw(bgSpr);
	map.draw(target);
	for (int i = 0; i < triggers.size(); i++)
	{
		triggers[i].rb.resize({triggers[i].rect.width, triggers[i].rect.height});
		if (Window::getVar("debug")) triggers[i].rb.draw(&screen);
	}
	for (int i = 0; i < ents.size(); i++)
	{
		ents[i]->draw(&screen);
	}
	for (int i = 0; i < items.size(); i++)
	{
		items[i].draw();
	}
	cam.update(bgSpr.getGlobalBounds().getSize());
	screen.display();
	sf::Sprite spr(screen.getTexture());
	Window::setView(cam.view);
	Window::draw(spr, false);
	Window::drawScreen();
	Window::setVar("mouseGameX", Input::getMousePos(true).x);
	Window::setVar("mouseGameY", Input::getMousePos(true).y);
}

World::Trigger::Trigger()
{
	rect = {0, 0, 0, 0};
	rb = Rigidbody();
	clear();
}

World::Trigger::Trigger(sf::FloatRect r, sf::String prompt)
{
	rect = r;
	rb = Rigidbody();
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
	rb.create(
		{rect.left + rect.width / 2, rect.top + rect.height / 2},
		{rect.width, rect.height},
		getVar("friction"),
		getVar("mass"),
		getVar("restitution"),
		getVar("angle"),
		getVar("fixedAngle"),
		!getVar("static"),
		getVar("collision")
	);
}

World::Camera::Camera()
{
	view = sf::View({0, 0, 0, 0});
	owner = "";
	offset = {0, 0};
}

World::Camera::Camera(sf::Vector2f size, sf::Vector2f os, sf::String name)
{
	view = sf::View({0, 0, size.x, size.y});
	offset = os;
	owner = name;
}

void World::Camera::update(sf::Vector2f mapSize)
{
	auto *e = getEnt(owner);
	sf::Vector2f pos;
	if (e == nullptr)
	{
		pos = view.getSize() / 2.0f;
	}
	else
	{
		pos = e->getPosition() + offset;
	}
	sf::Vector2f min = view.getSize() / 2.0f;
	sf::Vector2f max = mapSize - (view.getSize() / 2.0f);
	if (view.getSize().x > mapSize.x) { min.x = max.x = mapSize.x / 2; }
	if (view.getSize().y > mapSize.y) { min.y = max.y = mapSize.y / 2; }
	view.setCenter(tr::lerpVec(
		view.getCenter(),
		tr::clampVec(pos, min, max),
		0.05
	));
}

void World::setActive(bool a) { active = a; }
bool World::getActive() { return active; }

World::SoundPlayer::SoundPlayer()
{
	sound = sf::Sound();
}

World::SoundPlayer::SoundPlayer(std::string file, sf::Vector2f pos, float distance)
{
	sound = sf::Sound();
	sound.setBuffer(*AssetManager::getSound(file));
	sound.setPosition(pos.x, 0, pos.y);
	sound.setVolume(Window::getVar("sfxVolume"));
	sound.setMinDistance(distance);
	sound.setAttenuation(1);
	sound.setLoop(false);
}

void World::playSound(std::string file, sf::Vector2f pos, float distance)
{
	lvls[currentLevel].sounds.push_back({file, pos, distance});
	lvls[currentLevel].sounds[lvls[currentLevel].sounds.size() - 1].sound.play();
}

std::vector<World::Trigger*> World::getTriggers()
{
	std::vector<World::Trigger*> rbs;
	for (int i = 0; i < lvls[currentLevel].triggers.size(); i++)
	{
		rbs.push_back(&lvls[currentLevel].triggers[i]);
	}
	return rbs;
}

std::vector<Entity*> World::getEntsWithVar(sf::String name, sf::String value)
{
	std::vector<Entity*> e;
	for (int i = 0; i < ents.size(); i++)
	{
		if (ents[i].getVar(name) == value) { e.push_back(&ents[i]); }
	}
	return e;
}

std::vector<Entity*> World::getEntsWithVar(sf::String name, float value)
{
	std::vector<Entity*> e;
	for (int i = 0; i < ents.size(); i++)
	{
		if (ents[i].getVar(name) == value) { e.push_back(&ents[i]); }
	}
	return e;
}

World::Light::Light()
{
	pos = {0, 0};
	dist = 0;
	clr = {0, 0, 0, 0};
}

World::Light::Light(sf::Vector2f position, float distance, sf::Color color)
{
	pos = position;
	dist = distance;
	clr = color;
}

Entity *World::getCameraOwner()
{
	return getEnt(lvls[currentLevel].cam.owner);
}

World::FallenItem::FallenItem()
{
	item = Inventory::ItemEntry();
	rb.reset();
	cooldown.restart();
}

World::FallenItem::FallenItem(Inventory::ItemEntry entry, sf::Vector2f start, sf::Vector2f force)
{
	item = entry;
	rb.create({start.x, start.y}, {1, 1}, 0.2, 0.75, 0.5, 0, true, true, -1);
	impulse = force;
	cooldown.restart();
}

void World::FallenItem::draw()
{
	float scale = 2;
	item.item.updateSpr();
	rb.resize({item.item.spr.getTextureRect().width * scale, item.item.spr.getTextureRect().height * scale});
	if (!rb.getBody()) { rb.initBody(); }
	if (!rb.getBody()->GetLinearVelocity().x) { rb.getBody()->ApplyLinearImpulseToCenter({impulse.x, impulse.y}, true); impulse = {0, 0}; }
	item.item.spr.setPosition(rb.getPosition().x, rb.getPosition().y);
	item.item.spr.setRotation(rb.getAngle());
	item.item.spr.setOrigin((sf::Vector2f)item.item.spr.getTextureRect().getSize() / 2.0f);
	item.item.spr.setScale(scale, scale);
	screen.draw(item.item.spr);
	if (Window::getVar("debug"))
	{
		sf::RectangleShape rect;
		rect.setSize(item.item.spr.getGlobalBounds().getSize());
		rect.setPosition(item.item.spr.getGlobalBounds().getPosition());
		rect.setFillColor(sf::Color(0, 0, 0, 0));
		rect.setOutlineColor(sf::Color::Red);
		rect.setOutlineThickness(-2);
		screen.draw(rect);
	}
}

void World::throwItem(Inventory::Item itm, Entity *entity)
{
	auto rotation = entity->getVar("rotation").num;
	auto position = entity->getPosition() + sf::Vector2f((entity->getHitbox().width) * rotation, 0);
	auto impulse = sf::Vector2f(
		entity->getVar("dx") + rotation,
		-abs(entity->getVar("dy")) - 0.5
	);
	World::lvls[World::currentLevel].items.push_back(FallenItem({itm, 1}, position, impulse));
}

World::Control::Control() { entName = ctrlID = ""; }

World::Control::Control(sf::String ent, sf::String ctrl) { entName = ent; ctrlID = ctrl; }

void tr::execute(sf::String cmd)
{
	auto args = tr::splitStr(cmd, "-");
	if (args[0] == "window")
	{
		if (args[1] == "close") { Window::close(); }
	}
	else if (args[0] == "world")
	{
		if (args[1] == "load") { World::loadFromFile(args[2]); }
		if (args[1] == "playSound")
		{
			World::playSound(args[2], {
				std::stof(args[3].toAnsiString()), std::stof(args[4].toAnsiString())
			}, std::stof(args[5].toAnsiString()));
		}
	}
	else if (args[0] == "ui")
	{
		if (args[1] == "setFrame") { UI::setFrame(args[2]); }
		else if (args[1] == "load") { UI::loadFromFile(args[2], true); }
	}
	else if (args[0] == "talk")
	{
		if (args[1] == "setPhrase")
		{
			Talk::getCurrentDialogue()->currentPhrase = args[2];
		}
		else if (args[1] == "setDialogue")
		{
			Talk::currentDialogue = args[2];
		}
		else if (args[1] == "active")
		{
			Talk::active = std::stoi(args[2].toAnsiString());
		}
		else if (args[1] == "load")
		{
			Talk::loadFromFile(args[2]);
		}
		else if (args[1] == "restart")
		{
			Talk::restart();
		}
	}
	else if (args[0] == "input")
	{
		if (args[1] == "active") { Input::active = std::stoi(args[2].toAnsiString()); }
	}
}