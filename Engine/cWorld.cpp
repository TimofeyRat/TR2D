#include "hWorld.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hInput.hpp"
#include "hInventory.hpp"
#include "hDialogue.hpp"
#include "hUI.hpp"
#include "hParticles.hpp"

#include <iostream>

std::map<sf::String, sf::String> World::ents;
sf::RenderTexture World::screen;
std::vector<World::Level> World::lvls;
int World::currentLevel, World::nextLevel;
sf::Music World::music;
bool World::active;
sf::String World::currentMusic;
float World::brightness, World::musicVolume;

void World::init()
{
	ents.clear();
	for (auto path : AssetManager::getTexts(".trent"))
	{
		pugi::xml_document file;
		file.load_string(AssetManager::getText(path).toWideString().c_str());
		ents[Entity(file.first_child()).name] = path;
	}
	lvls.clear();
	currentLevel = 0;
	music.setRelativeToListener(true);
	music.stop();
	music.setVolume(Window::getVar("musicVolume"));
	active = false;
	currentMusic = "";
	brightness = 0;
	musicVolume = 100;
	
	Inventory::init();
	ParticleSystem::init();
}

void World::loadFromFile(std::string filename)
{
	init();
	pugi::xml_document doc;
	doc.load_string(AssetManager::getText(filename).toWideString().c_str());
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
		level.map.scale = map.attribute(L"scale").as_float();
		auto tilemap = tr::splitStr(map.text().as_string(), "x");
		for (int y = 0; y < level.map.mapSize.y; y++)
		{
			for (int x = 0; x < level.map.mapSize.x; x++)
			{
				level.map.tiles[x][y] = std::stoi(tilemap[y * level.map.mapSize.x + x].toAnsiString());
			}
		}
		level.bgTex = AssetManager::getTexture(pugi::as_utf8(lvl.child(L"background").attribute(L"path").as_string()));
		auto bgBounds = tr::splitStr(lvl.child(L"background").attribute(L"bounds").as_string(), " ");
		level.bgBounds = {
			std::stof(bgBounds[0].toAnsiString()), std::stof(bgBounds[1].toAnsiString()),
			std::stof(bgBounds[2].toAnsiString()), std::stof(bgBounds[3].toAnsiString())
		};
		level.musicFilename = pugi::as_utf8(lvl.child(L"music").attribute(L"path").as_string());
		level.musicVolume = lvl.child(L"music").attribute(L"volume").as_float();
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
			level.spawners.push_back(Spawner(ent.attribute(L"name").as_string(), {std::stof(pos[0].toAnsiString()), std::stof(pos[1].toAnsiString())}));
			level.ents.push_back(Entity(getEntFile(ent.attribute(L"name").as_string())));
		}
		auto gravity = lvl.child(L"gravity").attribute(L"value").as_string();
		level.gravity = {
			std::stof(tr::splitStr(gravity, " ")[0].toAnsiString()),
			std::stof(tr::splitStr(gravity, " ")[1].toAnsiString())
		};
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
			level.triggers.push_back(Trigger(prompt));
		}
		for (auto control : lvl.children(L"control"))
		{
			level.controls.push_back(Control(
				control.attribute(L"ent").as_string(),
				control.attribute(L"controller").as_string()
			));
		}
		for (auto script : lvl.children(L"script"))
		{
			level.scripts.push_back(ScriptObject(
				pugi::as_utf8(script.attribute(L"file").as_string()),
				script.attribute(L"mainFunc").as_string(),
				script.attribute(L"executor").as_string()
			));
		}
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
	if (currentLevel != nextLevel)
	{
		brightness = tr::lerp(brightness, 0, 5 * Window::getDeltaTime());
		if (brightness == tr::clamp(brightness, 0, 1))
		{
			currentLevel = nextLevel;
		}
	}
	else if (brightness < 255) brightness = tr::lerp(brightness, 255, 5 * Window::getDeltaTime());
	music.setVolume(brightness / 255 * musicVolume);
	if (getCurrentLevel()->musicFilename != currentMusic)
	{
		currentMusic = getCurrentLevel()->musicFilename;
		music.openFromFile(currentMusic);
		musicVolume = getCurrentLevel()->musicVolume * Window::getVar("musicVolume") / 100.0f;
		music.play();
	}
	auto *lvl = getCurrentLevel();
	if (lvl) lvl->update();
}

void World::draw()
{
	if (currentLevel < 0 || currentLevel >= lvls.size() || !active) return;
	lvls[currentLevel].draw(&screen);
}

sf::RenderTexture *World::getScreen() { return &screen; }

sf::String World::getEntFile(sf::String name) { return ents[name]; }

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

sf::Vector2f World::Map::getPixelSize()
{
	return sf::Vector2f(
		mapSize.x * tileSize.x * scale,
		mapSize.y * tileSize.y * scale
	);
}

World::Level::Level()
{
	world = nullptr;
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
	items.clear();
	controls.clear();
	ents.clear();
	spawners.clear();
	scripts.clear();
	bgBounds = {0, 0, 0, 0};
	started = false;
	if (world != nullptr) { delete world; }
	musicVolume = 100;
	clear();
}

Entity *World::Level::getEntity(sf::String name)
{
	for (int i = 0; i < ents.size(); i++)
	{
		if (ents[i].name == name) { return &ents[i]; }
	}
	return nullptr;
}

void World::Level::update()
{
	if (!started)
	{
		if (world) { delete world; }
		world = new b2World(gravity);
		for (int i = 0; i < triggers.size(); i++)
		{
			triggers[i].rb.reset(world);
		}
		for (int i = 0; i < ents.size(); i++)
		{
			ents[i].getRigidbody()->reset(world);
		}
		for (int i = 0; i < spawners.size(); i++)
		{
			getEntity(spawners[i].name)->setPosition(spawners[i].pos);
		}
		started = true;
	}
	world->SetGravity(gravity);
	world->Step(Window::getDeltaTime(), 12, 8);
	auto musicStatus = music.getStatus();
	if (musicStatus == sf::Music::Status::Stopped)
	{
		music.openFromFile(musicFilename);
		musicVolume = musicVolume * Window::getVar("musicVolume") / 100.0f;
		music.play();
	}
	else if (musicStatus == sf::Music::Status::Paused) { music.play(); }
	for (int i = 0; i < sounds.size(); i++)
	{
		auto *s = &sounds[i].sound;
		if (s->getStatus() == sf::Sound::Status::Paused) { s->play(); }
		if (s->getStatus() == sf::Sound::Status::Stopped)
		{
			if (s->getPlayingOffset().asSeconds() > 0) sounds.erase(sounds.begin() + i);
			else sounds[i].sound.play();
		}
	}
	for (int i = 0; i < scripts.size(); i++)
	{
		if (tr::strContains(scripts[i].executor, "trigger"))
		{
			auto t = tr::splitStr(scripts[i].executor, "_")[1];
			for (int j = 0; j < triggers.size(); j++)
			{
				if (triggers[j].getVar("name") == t) { scripts[i].src.execute(scripts[i].mainFunc, &triggers[j]); }
			}
		}
		else if (scripts[i].executor != "world")
		{
			auto executors = tr::splitStr(scripts[i].executor, ";");
			for (int j = 0; j < executors.size(); j++)
			{
				scripts[i].src.execute(scripts[i].mainFunc, getEntity(executors[j]));
			}
		}
		else scripts[i].src.execute(scripts[i].mainFunc, this);
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
		ents[i].update();
		// if (!ents[i].weapon.meleeOrRange && ents[i].getVar("attacking"))
		// {
		// 	auto r = ents[i].weapon.spr.getGlobalBounds();
		// 	for (int j = 0; j < ents.size(); j++)
		// 	{
		// 		if (i != j &&
		// 			r.intersects(ents[j].getHitbox()) &&
		// 			ents[j].getVar("noHurtTimer") >= ents[j].getVar("damageCD"))
		// 		{
		// 			for (int k = 0; k < ents[i].weapon.effects.size(); k++)
		// 			{
		// 				ents[j].addEffect(ents[i].weapon.effects[k]);
		// 			}
		// 			ents[j].setVar("noHurtTimer", 0);
		// 		}
		// 	}
		// }
	}
	auto *camOwner = getEntity(cam.owner);
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
					items[i].rb.destroy(world);
					items[j].item.count += items[i].item.count;
					items.erase(items.begin() + i--);
				}
			}
			if (camOwner->getHitbox().intersects(items[j].item.item.spr.getGlobalBounds()) && items[j].cooldown.getElapsedTime().asSeconds() >= 1.5f)
			{
				Inventory::addItem(items[j].item.item.type, items[j].item.item.id, items[j].item.count);
				items[j].rb.destroy(world);
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
	if (bgTex != nullptr) bgSpr.setTexture(*bgTex, false);
	bgSpr.setScale(
		bgBounds.width / bgTex->getSize().x,
		bgBounds.height / bgTex->getSize().y
	);
	target->draw(bgSpr);
	map.draw(target);
	for (int i = 0; i < triggers.size(); i++)
	{
		triggers[i].rb.resize(world, {triggers[i].rect.width, triggers[i].rect.height});
		if (Window::getVar("debug")) triggers[i].rb.draw(&screen);
	}
	for (int i = 0; i < ents.size(); i++)
	{
		ents[i].draw(&screen);
	}
	for (int i = 0; i < items.size(); i++)
	{
		items[i].draw(world);
	}
	cam.update(mapSize);
	screen.display();
	sf::Sprite spr(screen.getTexture());
	spr.setColor({brightness, brightness, brightness, 255});
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

World::Trigger::Trigger(sf::String prompt)
{
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
	auto pos = tr::splitStr(getVar("pos"), " ");
	auto size = tr::splitStr(getVar("size"), " ");
	rect.left = std::stof(pos[0].toAnsiString()); rect.top = std::stof(pos[1].toAnsiString());
	rect.width = std::stof(size[0].toAnsiString()); rect.height = std::stof(size[1].toAnsiString());
	setVar("x", rect.left); setVar("y", rect.top); setVar("w", rect.width); setVar("h", rect.height);
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
	auto *e = World::lvls[World::currentLevel].getEntity(owner);
	sf::Vector2f pos;
	if (e == nullptr)
	{
		pos = offset;
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

Entity *World::getCameraOwner()
{
	return getCurrentLevel()->getEntity(lvls[currentLevel].cam.owner);
}

World::FallenItem::FallenItem()
{
	item = Inventory::ItemEntry();
	rb.reset(nullptr);
	cooldown.restart();
}

World::FallenItem::FallenItem(b2World *w, Inventory::ItemEntry entry, sf::Vector2f start, sf::Vector2f force)
{
	item = entry;
	rb.create({start.x, start.y}, {1, 1}, 0.2, 0.75, 0.5, 0, true, true, -1);
	impulse = force;
	cooldown.restart();
}

void World::FallenItem::draw(b2World *world)
{
	float scale = 2;
	item.item.updateSpr();
	rb.resize(world, {item.item.spr.getTextureRect().width * scale, item.item.spr.getTextureRect().height * scale});
	if (!rb.getBody()) { rb.reset(world); }
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
	World::lvls[World::currentLevel].items.push_back(FallenItem(entity->getRigidbody()->getBody()->GetWorld(), {itm, 1}, position, impulse));
}

World::Control::Control() { entName = ctrlID = ""; }

World::Control::Control(sf::String ent, sf::String ctrl) { entName = ent; ctrlID = ctrl; }

World::Spawner::Spawner()
{
	name = "";
	pos = {0, 0};
}

World::Spawner::Spawner(sf::String ent, sf::Vector2f xy)
{
	name = ent;
	pos = xy;
}

World::ScriptObject::ScriptObject()
{
	src = Script();
	mainFunc = executor = "";
}

World::ScriptObject::ScriptObject(std::string filename, sf::String MAIN, sf::String owner)
{
	src = Script(filename);
	mainFunc = MAIN;
	executor = owner;
}

World::Level *World::getCurrentLevel() { return &lvls[currentLevel]; }

void World::setCurrentLevel(sf::String name)
{
	for (int i = 0; i < lvls.size(); i++)
	{
		if (lvls[i].name == name) { nextLevel = i; }
	}
}

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
		else if (args[1] == "playSound")
		{
			World::playSound(args[2], {
				std::stof(args[3].toAnsiString()), std::stof(args[4].toAnsiString())
			}, std::stof(args[5].toAnsiString()));
		}
		else if (args[1] == "setLevel") { World::setCurrentLevel(args[2]); }
		else if (args[1] == "setActive") { World::setActive(std::stoi(args[2].toAnsiString())); }
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