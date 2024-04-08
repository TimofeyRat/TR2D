#include "hWorld.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hInput.hpp"
#include "hInventory.hpp"
#include "hDialogue.hpp"
#include "hUI.hpp"

#include <iostream>

std::map<sf::String, sf::String> World::ents;
sf::RenderTexture World::screen;
std::vector<World::Level> World::lvls;
int World::currentLevel, World::nextLevel;
sf::Music World::music;
bool World::active;
sf::String World::currentMusic;
float World::brightness, World::musicVolume;

WorldCL::WorldCL() {}

void WorldCL::BeginContact(b2Contact *contact)
{
	auto p1 = contact->GetFixtureA()->GetBody()->GetUserData().pointer;
	auto p2 = contact->GetFixtureB()->GetBody()->GetUserData().pointer;
	if (p1 == 0 || p2 == 0) return;
	auto s1 = *(sf::String*)p1;
	auto s2 = *(sf::String*)p2;
	if (s1.isEmpty() || s2.isEmpty()) return;
	auto b1 = tr::splitStr(s1, "_");
	auto b2 = tr::splitStr(s2, "_");
	if (b1[0] == "trigger" && b2[0] == "particle") ParticleTrigger(std::stoi(b2[1].toAnsiString()), b1[1]);
	if (b1[0] == "particle" && b2[0] == "trigger") ParticleTrigger(std::stoi(b1[1].toAnsiString()), b2[1]);
	if (b1[0] == "ent" && b2[0] == "particle") ParticleEntity(std::stoi(b2[1].toAnsiString()), b1[1]);
	if (b1[0] == "particle" && b2[0] == "ent") ParticleEntity(std::stoi(b1[1].toAnsiString()), b2[1]);
	if (b1[0] == "trigger" && b2[0] == "ent") EntityTrigger(b2[1], b1[1]);
	if (b1[0] == "ent" && b2[0] == "trigger") EntityTrigger(b1[1], b2[1]);
}

void WorldCL::EndContact(b2Contact *contact)
{
	auto p1 = contact->GetFixtureA()->GetBody()->GetUserData().pointer;
	auto p2 = contact->GetFixtureB()->GetBody()->GetUserData().pointer;
	if (p1 == 0 || p2 == 0) return;
	auto s1 = *(sf::String*)p1;
	auto s2 = *(sf::String*)p2;
	if (s1.isEmpty() || s2.isEmpty()) return;
	auto b1 = tr::splitStr(s1, "_");
	auto b2 = tr::splitStr(s2, "_");
	if (b1[0] == "trigger" && b2[0] == "ent") EntityTrigger(b2[1], b1[1], false);
	if (b1[0] == "ent" && b2[0] == "trigger") EntityTrigger(b1[1], b2[1], false);
}

void WorldCL::ParticleTrigger(int partID, sf::String triggerName, bool start)
{
	auto *part = &World::getCurrentLevel()->parts[partID];
	auto *trigger = World::getCurrentLevel()->getTrigger(triggerName);
	part->destroyed = true;
}

void WorldCL::ParticleEntity(int partID, sf::String entName, bool start)
{
	auto *part = &World::getCurrentLevel()->parts[partID];
	auto *ent = World::getCurrentLevel()->getEntity(entName);
	for (int i = 0; i < part->effects.size(); i++)
	{
		ent->addEffect(part->effects[i]);
	}
	part->destroyed = true;
}

void WorldCL::EntityTrigger(sf::String entName, sf::String triggerName, bool start)
{
	auto *lvl = World::getCurrentLevel();
	auto *ent = lvl->getEntity(entName);
	auto *trigger = lvl->getTrigger(triggerName);
	if (entName == lvl->cam.owner)
	{
		if (ent->getHitbox().intersects(trigger->rect) &&
			trigger->hasVar("cmd") &&
			(trigger->getVar("used") < trigger->getVar("usages") ||
			trigger->getVar("usages") == 0))
		{
			lvl->setVar("showInteraction", 1);
			lvl->setVar("interactableTrigger", triggerName);
		}
		else
		{
			lvl->setVar("showInteraction", 0);
			lvl->setVar("interactableTrigger", "");
		}
	}
}

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
		for (auto gen : lvl.children(L"particles"))
		{
			auto min = tr::splitStr(gen.attribute(L"min").as_string(), " ");
			auto max = tr::splitStr(gen.attribute(L"max").as_string(), " ");
			auto rect = tr::splitStr(gen.attribute(L"rect").as_string(), " ");
			level.partGens.push_back(ParticleGenerator(
				gen.attribute(L"type").as_string(),
				gen.attribute(L"spawnRule").as_string(),
				gen.attribute(L"velocity").as_string(),
				{std::stof(min[0].toAnsiString()), std::stof(min[1].toAnsiString())},
				{std::stof(max[0].toAnsiString()), std::stof(max[1].toAnsiString())},
				gen.attribute(L"timer").as_float(),
				{
					std::stof(rect[0].toAnsiString()), std::stof(rect[1].toAnsiString()),
					std::stof(rect[2].toAnsiString()), std::stof(rect[3].toAnsiString())
				}
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
	partGens.clear();
	parts.clear();
	bgBounds = {0, 0, 0, 0};
	started = false;
	if (world != nullptr) { delete world; }
	musicVolume = 100;
	clear();
	cl = WorldCL();
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
		world->SetContactListener(&cl);
		for (int i = 0; i < triggers.size(); i++)
		{
			triggers[i].rb.reset(world);
			triggers[i].rb.resize(world, {triggers[i].rect.width, triggers[i].rect.height});
			triggers[i].rb.setUserData(sf::String("trigger_") + triggers[i].getVar("name"));
			if (triggers[i].getVar("collision") == 0) triggers[i].rb.getBody()->GetFixtureList()->SetSensor(true);
		}
		for (int i = 0; i < ents.size(); i++)
		{
			ents[i].getRigidbody()->reset(world);
			ents[i].getRigidbody()->setUserData(sf::String("ent_") + ents[i].name);
		}
		for (int i = 0; i < spawners.size(); i++)
		{
			getEntity(spawners[i].name)->setPosition(spawners[i].pos);
		}
		setVar("w", map.getPixelSize().x);
		setVar("h", map.getPixelSize().y);
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
	for (int i = 0; i < partGens.size(); i++)
	{
		auto *pg = &partGens[i];
		{
			auto rule = tr::splitStr(pg->spawnRule, "-");
			if (rule[0] == "chance")
			{
				auto chance = std::stof(rule[1].toAnsiString());
				int count = 1;
				if (rule.size() == 3) count = std::stoi(rule[2].toAnsiString());
				for (int j = 0; j < count; j++)
				{
					sf::Vector2f pos = {
						tr::randBetween(pg->spawnRect.left, pg->spawnRect.left + pg->spawnRect.width),
						tr::randBetween(pg->spawnRect.top, pg->spawnRect.top + pg->spawnRect.height)
					}, speed = {
						tr::randBetween(pg->minVel.x, pg->maxVel.x),
						tr::randBetween(pg->minVel.y, pg->maxVel.y)
					};
					if ((float)rand() / RAND_MAX * 100 <= chance)
					{
						auto part = ParticleSystem::createParticle(world, pg->temp, pos, speed);
						part.life = pg->lifeTime;
						parts.push_back(part);
						parts[parts.size() - 1].rb.setUserData("particle_" + std::to_string(parts.size() - 1));
					}
				}
			}
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
	for (int i = 0; i < parts.size(); i++)
	{
		auto *p = &parts[i];
		if (p->life > 0)
		{
			p->timer += Window::getDeltaTime();
			if (p->timer >= p->life) { p->destroyed = true; }
		}
		if (!p->physics)
		{
			p->shape.move(p->speed * Window::getDeltaTime());
			for (int j = 0; j < triggers.size(); j++)
			{
				if (triggers[j].rect.intersects(p->shape.getGlobalBounds()))
				{
					p->destroyed = true;
					break;
				}
			}
			if (!p->destroyed) for (int j = 0; j < ents.size(); j++)
			{
				if (ents[j].getHitbox().intersects(p->shape.getGlobalBounds()))
				{
					for (int k = 0; k < p->effects.size(); k++)
					{
						ents[j].addEffect(p->effects[k]);
					}
					p->destroyed = true;
					break;
				}
			}
		}
		if (p->destroyed)
		{
			p->rb.destroy(world);
			std::swap(parts[i], parts[parts.size() - 1]);
			parts.pop_back();
		}
	}
	for (int i = 0; i < ents.size(); i++) { ents[i].update(); }
	auto *camOwner = getEntity(cam.owner);
	if (camOwner)
	{
		if (getVar("showInteraction") && camOwner->getVar("interacting"))
		{
			auto *t = getTrigger(getVar("interactableTrigger"));
			auto prompt = t->getVar("cmd").str;
			if (!prompt.isEmpty())
			{
				auto cmd = tr::splitStr(prompt, "|");
				for (int j = 0; j < cmd.size(); j++)
				{
					tr::execute(cmd[j]);
				}
				t->setVar("used", t->getVar("used") + 1);
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
	cam.update(mapSize);
	sf::Listener::setPosition(cam.view.getCenter().x, 0, cam.view.getCenter().y);
	sf::FloatRect camRect = {cam.view.getCenter() - cam.view.getSize() / 2.0f, cam.view.getSize()};
	for (int i = 0; i < ents.size(); i++)
	{
		if (ents[i].getHitbox().intersects(camRect)) ents[i].draw(&screen);
	}
	for (int i = 0; i < items.size(); i++)
	{
		items[i].draw(world);
	}
	for (int i = 0; i < parts.size(); i++)
	{
		auto *p = &parts[i];
		if (p->physics)
		{
			p->shape.setPosition(p->rb.getPosition().x, p->rb.getPosition().y);
			p->rb.getBody()->SetTransform(p->rb.getBody()->GetPosition(), atan2(
				p->rb.getBody()->GetLinearVelocity().y,
				p->rb.getBody()->GetLinearVelocity().x
			) - 90 * tr::DEGTORAD);
			p->shape.setRotation(p->rb.getAngle());
			p->rb.setUserData("particle_" + std::to_string(i));
		}
		p->shape.setOrigin(p->shape.getLocalBounds().getSize() / 2.0f);
		if (p->shape.getGlobalBounds().intersects(camRect)) screen.draw(p->shape);
	}
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
	if (!rb.getBody()) { rb.reset(world); rb.setUserData(sf::String("item_") + item.item.type + ":" + item.item.id); }
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
	auto position = entity->getPosition() + sf::Vector2f((entity->getHitbox().width / 2) * rotation, 0);
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

World::ParticleGenerator::ParticleGenerator()
{
	spawnRect = {0, 0, 0, 0};
	velType = spawnRule = temp = "";
	minVel = maxVel = {0, 0};
	lifeTime = timer = 0;
}

World::ParticleGenerator::ParticleGenerator(sf::String name, sf::String spawn, sf::String vel, sf::Vector2f min, sf::Vector2f max, float Timer, sf::FloatRect rect)
{
	temp = name;
	spawnRule = spawn;
	velType = vel;
	minVel = min;
	maxVel = max;
	lifeTime = Timer;
	timer = 0;
	spawnRect = rect;
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
	else if (args[0] == "inv")
	{
		if (args[1] == "clear") { Inventory::inv.clear(); }
		else if (args[1] == "addItem") { Inventory::addItem(args[2]); }
	}
	else if (args[0] == "setWeapon")
	{
		auto *ent = World::getCurrentLevel()->getEntity(args[1]);
		if (ent == nullptr) { return; }
		ent->weapon = Inventory::getWeapon(args[2]);
	}
}

World::Trigger *World::Level::getTrigger(sf::String name)
{
	for (int i = 0; i < triggers.size(); i++)
	{
		if (triggers[i].getVar("name") == name) { return &triggers[i]; }
	}
	return nullptr;
}