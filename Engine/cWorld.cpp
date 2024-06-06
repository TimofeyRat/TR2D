#include "hWorld.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hInput.hpp"
#include "hInventory.hpp"
#include "hDialogue.hpp"
#include "hUI.hpp"
#include "hCutscene.hpp"
#include <iostream>
#include <math.h>

std::map<sf::String, sf::String> World::ents;
sf::RenderTexture World::screen;
std::vector<World::Level> World::lvls;
int World::currentLevel, World::nextLevel;
sf::Music World::music;
bool World::active;
sf::String World::currentMusic, World::currentFile;
float World::brightness, World::musicVolume;
sf::Shader World::mapShader, World::lightShader, World::objectsShader, World::entShader;

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
	if (part->colliding) part->destroyed = true;
}

void WorldCL::ParticleEntity(int partID, sf::String entName, bool start)
{
	auto *part = &World::getCurrentLevel()->parts[partID];
	auto *ent = World::getCurrentLevel()->getEntity(entName);
	for (int i = 0; i < part->effects.size(); i++)
	{
		ent->addEffect(part->effects[i]);
	}
	if (part->colliding) part->destroyed = true;
}

void WorldCL::EntityTrigger(sf::String entName, sf::String triggerName, bool start)
{
	auto *lvl = World::getCurrentLevel();
	auto *ent = lvl->getEntity(entName);
	auto *trigger = lvl->getTrigger(triggerName);
	if (entName != lvl->cam.owner) return;
	
	if (trigger->hasVar("enter") && start &&
		(trigger->getVar("entered") < trigger->getVar("enters") || !trigger->hasVar("enters")))
	{
		auto cmd = tr::splitStr(trigger->getVar("enter"), "|");
		for (int i = 0; i < cmd.size(); i++) { tr::execute(cmd[i]); }
		if (trigger->hasVar("enters")) trigger->setVar("entered", trigger->getVar("entered") + 1);
	}

	if (trigger->hasVar("exit") && !start && !ent->getHitbox().intersects(trigger->rect) &&
		(trigger->getVar("exited") < trigger->getVar("exits") || !trigger->hasVar("exits")))
	{
		auto cmd = tr::splitStr(trigger->getVar("exit"), "|");
		for (int i = 0; i < cmd.size(); i++) { tr::execute(cmd[i]); }
		if (trigger->hasVar("exits")) trigger->setVar("exited", trigger->getVar("exited") + 1);
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
	mapShader.loadFromMemory(AssetManager::getText(AssetManager::path + "global/world.frag"), sf::Shader::Fragment);
	lightShader.loadFromMemory(AssetManager::getText(AssetManager::path + "global/light.frag"), sf::Shader::Fragment);
	objectsShader.loadFromMemory(AssetManager::getText(AssetManager::path + "global/objects.frag"), sf::Shader::Fragment);
	entShader.loadFromMemory(AssetManager::getText(AssetManager::path + "global/ents.frag"), sf::Shader::Fragment);

	Inventory::init();
	ParticleSystem::init();
	CSManager::init();
}

void World::loadFromFile(std::string filename)
{
	currentFile = filename;
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
					prompt += name[0] + "=str=" + attr.as_string() + ";";
				}
				if (name[1] == "num")
				{
					prompt += name[0] + "=num=" + attr.as_string() + ";";
				}
			}
			if (!trigger.text().empty())
			{
				prompt += sf::String("inter=str=") + trigger.text().as_string();
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
		for (auto light : lvl.children(L"light"))
		{
			auto clr = tr::splitStr(light.attribute(L"color").as_string(), " ");
			auto pos = tr::splitStr(light.attribute(L"pos").as_string(), " ");
			level.lights.push_back(LightSource(
				{
					std::stof(clr[0].toAnsiString()), std::stof(clr[1].toAnsiString()),
					std::stof(clr[2].toAnsiString()), std::stof(clr[3].toAnsiString())
				},
				{
					std::stof(pos[0].toAnsiString()), std::stof(pos[1].toAnsiString())
				},
				light.attribute(L"radius").as_float(),
				light.attribute(L"angle").as_float(),
				light.attribute(L"field").as_float()
			));
		}
		for (auto script : lvl.children(L"script"))
		{
			level.scripts.push_back({
				script.attribute(L"path").as_string(),
				script.attribute(L"mainFunc").as_string()
			});
		}
		World::lvls.push_back(level);
	}
	active = true;
	setCurrentLevel(lvls[0].name);
}

void World::update()
{
	if (currentLevel < 0 || currentLevel >= lvls.size()) return;
	if (!active)
	{
		music.pause();
		lvls[currentLevel].sounds.clear();
		return;
	}
	if (currentLevel != nextLevel)
	{
		brightness = tr::clamp(tr::lerp(brightness, 0, 5 * Window::getDeltaTime()), 0, 255);
		if (brightness == tr::clamp(brightness, 0, 1))
		{
			currentLevel = nextLevel;
		}
	}
	else if (brightness < 255) brightness = tr::clamp(tr::lerp(brightness, 255, 5 * Window::getDeltaTime()), 0, 255);
	musicVolume = getCurrentLevel()->musicVolume * Window::getVar("musicVolume") / 100.0f;
	sf::String cutsceneMusic;
	if (auto d = Talk::getCurrentDialogue())
	if (auto p = d->getCurrentPhrase())
		cutsceneMusic = CSManager::getMusic(p->name);
	if (CSManager::active && !cutsceneMusic.isEmpty())
	{
		if (currentMusic != cutsceneMusic)
		{
			music.setVolume(tr::clamp(music.getVolume() - Window::getDeltaTime() * 100, 0, Window::getVar("musicVolume")));
		}
		else
		{
			music.setVolume(tr::clamp(music.getVolume() + Window::getDeltaTime() * 100, 0, Window::getVar("musicVolume")));
		}
		if (music.getVolume() == 0)
		{
			currentMusic = cutsceneMusic;
			if (!currentMusic.isEmpty()) music.openFromFile(currentMusic);
			music.play();
		}
	}
	else if (getCurrentLevel()->musicFilename != currentMusic)
	{
		currentMusic = getCurrentLevel()->musicFilename;
		music.openFromFile(currentMusic);
		music.play();
	}
	if (getCurrentLevel()->musicFilename == currentMusic && !CSManager::active) music.setVolume(brightness / 255 * musicVolume);
	auto *lvl = getCurrentLevel();
	if (CSManager::active && CSManager::current.x) { CSManager::worlds[CSManager::current.y].update(); }
	else lvl->cam.doUpdate = true;
	lvl->update();
	Input::active = !CSManager::active && !Talk::active;
}

void World::draw()
{
	if (currentLevel < 0 || currentLevel >= lvls.size() || !active) return;
	lvls[currentLevel].draw();
}

sf::RenderTexture *World::getScreen() { return &screen; }

sf::String World::getEntFile(sf::String name) { return ents[name]; }

World::Map::Map() { reset(); }

void World::Map::draw(sf::RenderTarget *target, const sf::RenderStates &states)
{
	tile.setTexture(*tileTex);
	tile.setScale(scale, scale);

	for (int x = 0; x < mapSize.x; x++)
	{
		for (int y = 0; y < mapSize.y; y++)
		{
			if (tiles[x][y] == 0) { continue; }
			tile.setTextureRect(tileRects[tiles[x][y] - 1]);
			tile.setPosition(x * tileSize.x * scale, y * tileSize.y * scale);
			target->draw(tile, states);
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

World::ScriptExecutor::ScriptExecutor()
{
	source = Script();
	func = "";
}

World::ScriptExecutor::ScriptExecutor(sf::String path, sf::String main)
{
	source.load(path);
	func = main;
}

World::Level::Level()
{
	world = nullptr;
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
	partGens.clear();
	parts.clear();
	partCurves.clear();
	lights.clear();
	scripts.clear();
	bgBounds = {0, 0, 0, 0};
	started = false;
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
		for (int i = 0; i < ents.size(); i++)
		{
			if (ents[i].getVar("posX") != 0 && ents[i].getVar("posY") != 0)
			{
				ents[i].setPosition({ents[i].getVar("posX"), ents[i].getVar("posY")});
			}
		}
		setVar("w", map.getPixelSize().x);
		setVar("h", map.getPixelSize().y);
		//Render background
		sf::RenderTexture render;
		render.create(getVar("w"), getVar("h"));
		render.clear({0, 0, 0, 0});
		if (bgTex != nullptr) bgSpr.setTexture(*bgTex, false);
		bgSpr.setScale(
			bgBounds.width / bgTex->getSize().x,
			bgBounds.height / bgTex->getSize().y
		);
		render.draw(bgSpr);
		map.draw(&render);
		render.display();
		auto temp = render.getTexture().copyToImage();
		temp.flipVertically();
		bg.loadFromImage(temp);
		//Render lights
		render.clear({0, 0, 0, 0});
		sf::RectangleShape basis((sf::Vector2f)render.getSize());
		std::vector<sf::Glsl::Vec2> lightPos(32, {0, 0});
		std::vector<sf::Glsl::Vec4> lightClr(32, {0, 0, 0, 0});
		std::vector<sf::Glsl::Vec3> lightData(32, {0, 0, 0});
		for (int i = 0; i < lights.size(); i++)
		{
			lightPos[i] = {
				lights[i].position.x,
				getVar("h") - lights[i].position.y
			};
			lightClr[i] = {
				(float)lights[i].color.r / 255.0f,
				(float)lights[i].color.g / 255.0f,
				(float)lights[i].color.b / 255.0f,
				(float)lights[i].color.a / 255.0f,
			};
			lightData[i] = {lights[i].radius, lights[i].angle, lights[i].field};
		}
		lightShader.setUniformArray("lightPos", lightPos.data(), 32);
		lightShader.setUniformArray("lightClr", lightClr.data(), 32);
		lightShader.setUniformArray("lightData", lightData.data(), 32);
		render.draw(basis, &lightShader);
		render.display();
		temp = render.getTexture().copyToImage();
		temp.flipVertically();
		lightMap.loadFromImage(temp);
		//Objects layer
		objects = new sf::RenderTexture();
		objects->create(getVar("w"), getVar("h"));
		//Entities layer
		entsLayer = new sf::RenderTexture();
		entsLayer->create(getVar("w"), getVar("h"));
		started = true;
		Window::resetTime();
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
		if (s->getPlayingOffset().asSeconds() >= s->getBuffer()->getDuration().asSeconds()) { sounds.erase(sounds.begin() + i); continue; }
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
		p->update();
		if (!p->physics && p->colliding)
		{
			for (int j = 0; j < triggers.size(); j++)
			{
				if (triggers[j].rect.intersects(p->shape.getGlobalBounds()) && triggers[j].getVar("collision").num != 0)
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
	for (int i = 0; i < partCurves.size(); i++)
	{
		partCurves[i].update();
		if (partCurves[i].getVar("joined")) { partCurves.erase(partCurves.begin() + i); }
	}
	for (int i = 0; i < scripts.size(); i++) { scripts[i].source.execute(scripts[i].func); }
	for (int i = 0; i < ents.size(); i++)
	{
		if (!ents[i].isAlive()) continue;
		if (ents[i].getRigidbody()->getUserData().isEmpty())
		{
			ents[i].getRigidbody()->reset(world);
			ents[i].getRigidbody()->setUserData("ent_" + ents[i].name);
		}
		ents[i].update();
		if (ents[i].weapon.id != "null" && ents[i].getVar("attacking"))
		{
			if (ents[i].weapon.effects.size()) for (int j = 0; j < ents.size(); j++)
			{
				if (i == j) continue;
				if (ents[j].getHitbox().intersects(ents[i].weapon.spr.getGlobalBounds()))
				if (ents[j].getVar("group").num != ents[i].getVar("group").num)
				if (ents[j].getVar("noHurtTimer") >= ents[j].getVar("damageCD"))
				{
					for (int k = 0; k < ents[i].weapon.effects.size(); k++)
					{
						ents[j].addEffect(ents[i].weapon.effects[k]);
					}
				}
			}
		}
	}
	auto *camOwner = getEntity(cam.owner);
	if (camOwner)
	{
		for (int i = 0; i < triggers.size(); i++)
		{
			if (triggers[i].rect.intersects(camOwner->getHitbox()) &&
				triggers[i].hasVar("inter") &&
				(triggers[i].hasVar("usages") ? (triggers[i].getVar("used") < triggers[i].getVar("usages")) : true) &&
				Input::active)
			{
				setVar("showInteraction", (CSManager::current.x ? !CSManager::active : true));
				setVar("interactableTrigger", triggers[i].getVar("name").str);
				break;
			}
			else
			{
				setVar("showInteraction", 0);
				setVar("interactableTrigger", "");	
			}
		}
		if (getVar("showInteraction") && camOwner->getVar("interacting"))
		{
			auto *t = getTrigger(getVar("interactableTrigger"));
			auto prompt = t->getVar("inter").str;
			if (!prompt.isEmpty())
			{
				auto cmd = tr::splitStr(prompt, "|");
				for (int j = 0; j < cmd.size(); j++)
				{
					tr::execute(cmd[j]);
				}
				if (t->hasVar("usages")) t->setVar("used", t->getVar("used") + 1);
			}
		}
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

void World::Level::draw()
{
	auto mapSize = map.getPixelSize();
	if (screen.getSize() != (sf::Vector2u)mapSize) { screen.create(mapSize.x, mapSize.y); }
	screen.clear();
	sf::Sprite mapSpr(bg);
	float random = (float)rand() / RAND_MAX;
	auto camOwner = getCameraOwner();
	mapShader.setUniform("rand", random);
	mapShader.setUniform("texture", bg);
	if (camOwner) mapShader.setUniform("camOwnerHP", camOwner->getVar("HP").num);
	mapShader.setUniform("lightMap", lightMap);
	screen.draw(mapSpr, &mapShader);
	cam.update(mapSize);
	setVar("camX", cam.view.getCenter().x);
	setVar("camY", cam.view.getCenter().y);
	sf::Listener::setPosition(cam.view.getCenter().x, 0, cam.view.getCenter().y);
	sf::FloatRect camRect = {cam.view.getCenter() - cam.view.getSize() / 2.0f, cam.view.getSize()};
	entsLayer->clear({0, 0, 0, 0});
	objects->clear({0, 0, 0, 0});
	std::vector<sf::Glsl::Vec4> entRects;
	for (int i = 0; i < items.size(); i++)
	{
		items[i].draw(world, objects);
	}
	for (int i = 0; i < partCurves.size(); i++)
	{
		partCurves[i].draw(objects);
	}
	for (int i = 0; i < ents.size(); i++)
	{
		if (ents[i].getHitbox().intersects(camRect) && ents[i].name != cam.owner)
		{
			ents[i].draw(entsLayer, &lightShader);
			auto hitbox = ents[i].getHitbox();
			entRects.push_back({
				hitbox.left, hitbox.top, hitbox.width, hitbox.height
			});
		}
	}
	if (camOwner) camOwner->draw(entsLayer);
	auto ownerRect = camOwner ? camOwner->getHitbox() : sf::FloatRect(0, 0, 0, 0);
	auto ownerVec4 = sf::Glsl::Vec4(
		ownerRect.left, ownerRect.top, ownerRect.width, ownerRect.height
	);
	for (int i = 0; i < parts.size(); i++)
	{
		auto *p = &parts[i];
		if (p->shape.getGlobalBounds().intersects(camRect)) objects->draw(p->shape);
	}
	if (Window::getVar("debug"))
	{
		for (int i = 0; i < triggers.size(); i++)
		{
			sf::RectangleShape rect(triggers[i].rect.getSize());
			rect.setOrigin(rect.getSize() / 2.0f);
			rect.setPosition(triggers[i].rb.getPosition().x, triggers[i].rb.getPosition().y);
			rect.setFillColor({0, 0, 0, 0});
			rect.setOutlineColor(sf::Color::Red);
			rect.setOutlineThickness(-2);
			screen.draw(rect);
		}
		if (CSManager::active) CSManager::drawDebug(&screen);
	}

	objects->display();
	objectsShader.setUniform("render", objects->getTexture());
	objectsShader.setUniform("lightMap", lightMap);
	if (camOwner) objectsShader.setUniform("camOwnerHP", camOwner->getVar("HP"));
	objectsShader.setUniform("rand", random);
	sf::Sprite objSpr(objects->getTexture());
	screen.draw(objSpr, &objectsShader);

	entsLayer->display();
	entShader.setUniform("render", entsLayer->getTexture());
	entShader.setUniform("lightMap", lightMap);
	if (camOwner) entShader.setUniform("camOwnerHP", camOwner->getVar("HP").num);
	entShader.setUniform("camOwnerRect", ownerVec4);
	entShader.setUniform("rand", random);
	std::vector<sf::Glsl::Vec4> camOwnerBones;
	std::vector<float> camOwnerBonesRot;
	if (camOwner) for (int i = 0; camOwner->getSkeleton()->getBone(i) != nullptr; i++)
	{
		auto b = camOwner->getSkeleton()->getBone(i);
		auto global = b->spr.getGlobalBounds();
		auto local = b->spr.getLocalBounds();
		camOwnerBones.push_back({
			global.left + global.width / 2,
			global.top + global.height / 2,
			local.width * b->spr.getScale().x, local.height * b->spr.getScale().y
		});
		camOwnerBonesRot.push_back(b->angle);
	}
	entShader.setUniformArray("camOwnerBones", camOwnerBones.data(), fmin(camOwnerBones.size(), 100));
	entShader.setUniformArray("camOwnerBonesRot", camOwnerBonesRot.data(), fmin(camOwnerBonesRot.size(), 100));
	entShader.setUniform("camOwnerBoneCount", (int)camOwnerBones.size());
	sf::Sprite entSpr(entsLayer->getTexture());
	screen.draw(entSpr, &entShader);

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
	doUpdate = true;
}

World::Camera::Camera(sf::Vector2f size, sf::Vector2f os, sf::String name)
{
	view = sf::View({0, 0, size.x, size.y});
	offset = os;
	owner = name;
	doUpdate = true;
}

void World::Camera::update(sf::Vector2f mapSize)
{
	if (!doUpdate) return;
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
		5 * Window::getDeltaTime()
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

void World::FallenItem::draw(b2World *world, sf::RenderTarget *target)
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
	target->draw(item.item.spr);
	if (Window::getVar("debug"))
	{
		sf::RectangleShape rect;
		rect.setSize(item.item.spr.getGlobalBounds().getSize());
		rect.setPosition(item.item.spr.getGlobalBounds().getPosition());
		rect.setFillColor(sf::Color(0, 0, 0, 0));
		rect.setOutlineColor(sf::Color::Red);
		rect.setOutlineThickness(-2);
		target->draw(rect);
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

World::Level *World::getCurrentLevel() { return &lvls[currentLevel]; }

void World::setCurrentLevel(sf::String name)
{
	for (int i = 0; i < lvls.size(); i++)
	{
		if (lvls[i].name == name) { nextLevel = i; }
	}
}

World::Level *World::getLevel(sf::String name)
{
	for (int i = 0; i < lvls.size(); i++)
	{
		if (lvls[i].name == name) return &lvls[i];
	}
	return nullptr;
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

World::ParticleCurve::ParticleCurve()
{
	curve.clear();
	parts.clear();
	type = Standalone;
	math = Set;
	clear();
}

void World::ParticleCurve::init(b2World *w, World::ParticleCurve::Type t, sf::String entA, sf::String entB, std::vector<sf::Vector2f> points, float count, sf::String name, sf::Glsl::Vec4 speed, float lt, float s, float l, float d)
{
	type = t;
	if (type == TwoEnts)
	{
		setVar("entA", entA);
		setVar("entB", entB);
		curve.push_back({0, 0});
		curve.insert(curve.end(), points.begin(), points.end());
		curve.push_back({0, 0});
	}
	if (type == Standalone) { curve = points; }
	parts.resize(count);
	for (int i = 0; i < count; i++)
	{
		parts[i] = ParticleSystem::createParticle(w, name, {0, 0}, {tr::randBetween(speed.x, speed.z), tr::randBetween(speed.y, speed.w)});
		parts[i].life = lt;
	}
	setVar("speed", s);
	setVar("length", l);
	setVar("duration", d);
}

void World::ParticleCurve::join()
{
	if (getVar("joined")) return;
	World::getCurrentLevel()->parts.insert(World::getCurrentLevel()->parts.end(), parts.begin(), parts.end());
	setVar("joined", 1);
}

void World::ParticleCurve::update()
{
	setVar("time", getVar("time") + Window::getDeltaTime());
	if (getVar("time") >= getVar("duration") && getVar("duration") > 0) { join(); setVar("joined", 1); return; }
	if (type == TwoEnts)
	{
		curve[0] = World::getCurrentLevel()->getEntity(getVar("entA"))->getPosition();
		curve[curve.size() - 1] = World::getCurrentLevel()->getEntity(getVar("entB"))->getPosition();
	}
	for (int i = 0; i < parts.size(); i++)
	{
		float t = (float)i / parts.size();
		auto p = tr::getBezierPoint(curve, t);
		if (parts[i].physics) { parts[i].rb.setPosition({p.x, p.y}); }
		else { parts[i].shape.setPosition(p); }
		parts[i].update();
	}
}

void World::ParticleCurve::draw(sf::RenderTarget *target)
{
	int start, end;
	switch (math)
	{
	case Set:
		{
			start = 0; end = parts.size();
		} break;
	case FadeIn:
		{
			if (getVar("count") < parts.size()) setVar("count", getVar("count") + getVar("speed") * Window::getDeltaTime());
			else setVar("count", parts.size());
			start = 0;
			end = getVar("count");
		} break;
	case FadeLerp:
		{
			setVar("count", tr::lerp(getVar("count"), parts.size(), getVar("speed") * Window::getDeltaTime()));
			start = 0;
			end = getVar("count");
		} break;
	case Pulse:
		{
			setVar("pos", getVar("pos") + getVar("speed") * Window::getDeltaTime());
			if (getVar("pos") > parts.size() + getVar("length")) { setVar("pos", -getVar("length")); }
			start = getVar("pos") - getVar("length");
			end = getVar("pos") + getVar("length");
		} break;
	default: break;
	}
	start = tr::clamp(start, 0, parts.size());
	end = tr::clamp(end, 0, parts.size());
	for (int i = start; i < end; i++)
	{
		if (!parts[i].destroyed) target->draw(parts[i].shape);
	}
}

World::LightSource::LightSource()
{
	color = sf::Color::White;
	position = {0, 0};
	radius = angle = field = 0;
}

World::LightSource::LightSource(sf::Color clr, sf::Vector2f pos, float r, float a, float f)
{
	color = clr;
	position = pos;
	radius = r;
	angle = a;
	field = f;
}

void tr::execute(sf::String cmd)
{
	auto args = tr::splitStr(cmd, " ");
	if (args[0] == "window")
	{
		if (args[1] == "close") { Window::close(); }
		else if (args[1] == "showCredits")
		{
			Window::setVar("showCredits", std::stoi(args[2].toAnsiString()));
		}
		else if (args[1] == "setStr")
		{
			Window::setVar(args[2], args[3]);
		}
		else if (args[1] == "setNum")
		{
			Window::setVar(args[2], std::stof(args[3].toAnsiString()));
		}
		else if (args[1] == "showHint")
		{
			Window::setVar("hint", args[2]);
			Window::setVar("hintTimer", std::stof(args[3].toAnsiString()));
		}
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
			Talk::active = 1;
		}
		else if (args[1] == "setDialogue")
		{
			Talk::currentDialogue = args[2];
			Talk::active = 1;
		}
		else if (args[1] == "active")
		{
			Talk::active = std::stoi(args[2].toAnsiString());
		}
		else if (args[1] == "init")
		{
			Talk::init();
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
		else if (args[1] == "addItem") { Inventory::addItem(args[2], args.size() == 4 ? std::stoi(args[3].toAnsiString()) : 1); }
	}
	else if (args[0] == "setWeapon")
	{
		auto *ent = World::getCurrentLevel()->getEntity(args[1]);
		if (ent == nullptr) { return; }
		ent->weapon = Inventory::getWeapon(args[2]);
	}
	else if (args[0] == "cutscene")
	{
		if (args[1] == "init") { CSManager::init(); }
		else if (args[1] == "start")
		{
			if (!CSManager::frames.size() && !CSManager::worlds.size()) { CSManager::init(); }
			CSManager::setCutscene(args[2]);
			if (!CSManager::current.x) World::setActive(false);
			CSManager::active = true;
			Input::active = false;
			Talk::currentDialogue = CSManager::getTalk();
			Talk::active = true;
		}
		else if (args[1] == "active") { CSManager::active = std::stoi(args[2].toAnsiString()); }
		else if (args[1] == "stop")
		{
			if (CSManager::current.x) { CSManager::shouldEnd = true; }
			else { CSManager::active = false; }
			World::setActive(true);
			Input::active = true;
			Talk::active = false;
		}
		else if (args[1] == "setClip")
		{
			CSManager::worlds[CSManager::current.y].currentChange = args[2];
		}
	}
	else if (args[0] == "spawnParticle")
	{
		if (args.size() == 12)
		{
			for (int i = 0; i < std::stoi(args[11].toAnsiString()); i++)
			{
				sf::Vector2f pos = {
					tr::randBetween(std::stof(args[1].toAnsiString()), std::stof(args[3].toAnsiString())),
					tr::randBetween(std::stof(args[2].toAnsiString()), std::stof(args[4].toAnsiString()))
				}, speed = {
					tr::randBetween(std::stof(args[5].toAnsiString()), std::stof(args[7].toAnsiString())),
					tr::randBetween(std::stof(args[6].toAnsiString()), std::stof(args[8].toAnsiString()))
				};
				auto lvl = World::getCurrentLevel();
				auto part = ParticleSystem::createParticle(lvl->world, args[9], pos, speed);
				part.life = std::stof(args[10].toAnsiString());
				lvl->parts.push_back(part);
				lvl->parts[lvl->parts.size() - 1].rb.setUserData("particle_" + std::to_string(lvl->parts.size() - 1));
			}
		}
		else
		{
			sf::Vector2f pos = {
				tr::randBetween(std::stof(args[1].toAnsiString()), std::stof(args[3].toAnsiString())),
				tr::randBetween(std::stof(args[2].toAnsiString()), std::stof(args[4].toAnsiString()))
			}, speed = {
				tr::randBetween(std::stof(args[5].toAnsiString()), std::stof(args[7].toAnsiString())),
				tr::randBetween(std::stof(args[6].toAnsiString()), std::stof(args[8].toAnsiString()))
			};
			auto lvl = World::getCurrentLevel();
			auto part = ParticleSystem::createParticle(lvl->world, args[9], pos, speed);
			part.life = std::stof(args[10].toAnsiString());
			lvl->parts.push_back(part);
			lvl->parts[lvl->parts.size() - 1].rb.setUserData("particle_" + std::to_string(lvl->parts.size() - 1));
		}
	}
	else if (args[0] == "particleCurve")
	{
		auto points = tr::splitStr(args[1], "|");
		std::vector<sf::Vector2f> curve;
		for (int i = 0; i < points.size(); i++)
		{
			auto p = tr::splitStr(points[i], "_");
			curve.push_back({
				std::stof(p[0].toAnsiString()),
				std::stof(p[1].toAnsiString())
			});
		}
		auto speed = tr::splitStr(args[3], "_");
		auto step = 1.0f / std::stof(args[2].toAnsiString());
		auto name = args[4];
		auto count = std::stof(args[5].toAnsiString());
		auto life = std::stof(args[6].toAnsiString());
		for (float t = 0; t <= 1; t += step)
		{
			auto pos = tr::getBezierPoint(curve, t);
			auto lvl = World::getCurrentLevel();
			for (int i = 0; i < count; i++)
			{
				sf::Vector2f s = {
					tr::randBetween(std::stof(speed[0].toAnsiString()), std::stof(speed[2].toAnsiString())),
					tr::randBetween(std::stof(speed[1].toAnsiString()), std::stof(speed[3].toAnsiString()))
				};
				auto part = ParticleSystem::createParticle(lvl->world, name, pos, s);
				part.life = life;
				lvl->parts.push_back(part);
				lvl->parts[lvl->parts.size() - 1].rb.setUserData("particle_" + std::to_string(lvl->parts.size() - 1));
			}
		}
	}
	else if (args[0] == "assets")
	{
		if (args[1] == "init") { AssetManager::init(); }
		else if (args[1] == "reload")
		{
			if (args[2] == "text") { AssetManager::reloadText(args[3]); }
			else if (args[2] == "texture") { AssetManager::reloadTexture(args[3]); }
			else if (args[2] == "sound") { AssetManager::reloadSound(args[3]); }
			else if (args[2] == "font") { AssetManager::reloadFont(args[3]); }
		}
	}
	else if (args[0] == "ent")
	{
		if (args[1] == "setNum")
		{
			World::getCurrentLevel()->getEntity(args[2])->setVar(args[3], std::stof(args[4].toAnsiString()));
		}
		else if (args[1] == "setStr")
		{
			World::getCurrentLevel()->getEntity(args[2])->setVar(args[3], args[4]);
		}
		else if (args[1] == "setPos")
		{
			World::getCurrentLevel()->getEntity(args[2])->setPosition({
				std::stof(args[3].toAnsiString()),
				std::stof(args[4].toAnsiString())
			});
		}
		else if (args[1] == "addNum")
		{
			auto e = World::getCurrentLevel()->getEntity(args[2]);
			e->setVar(args[3], e->getVar(args[3]) + std::stof(args[4].toAnsiString()));
		}
		else if (args[1] == "setAnim")
		{
			auto e = World::getCurrentLevel()->getEntity(args[2]);
			e->setVar("anim", args[3]);
			e->setVar("dontUpdateAnim", 1);
		}
	}
	else if (args[0] == "lvl")
	{
		if (args[1] == "setNum")
		{
			World::getCurrentLevel()->setVar(args[2], std::stof(args[3].toAnsiString()));
		}
		if (args[2] == "setStr")
		{
			World::getCurrentLevel()->setVar(args[2], args[3]);
		}
	}
	else if (args[0] == "saveGame")
	{
		std::cout << "Saving...\n";
		pugi::xml_document doc;
		World::saveGame(doc.append_child(L"world"));
		auto inv = doc.append_child(L"inventory");
		Inventory::save(inv);
		inv.append_attribute(L"weapon") = World::getCameraOwner()->weapon.id.toWideString().c_str();
		inv.append_attribute(L"bauble") = World::getCameraOwner()->bauble.id.toWideString().c_str();
		doc.save_file(sf::String(AssetManager::path + "global/save.trconf").toWideString().c_str());
	}
	else if (args[0] == "loadGame")
	{
		std::cout << "Loading...\n";
		AssetManager::reloadText(AssetManager::path + "global/save.trconf");
		pugi::xml_document doc;
		doc.load_string(AssetManager::getText(AssetManager::path + "global/save.trconf").toWideString().c_str());
		World::loadGame(doc.child(L"world"));
		auto inv = doc.child(L"inventory");
		Inventory::load(inv);
		World::getCameraOwner()->weapon = Inventory::getWeapon(inv.attribute(L"weapon").as_string());
		World::getCameraOwner()->bauble = Inventory::getBauble(inv.attribute(L"bauble").as_string());
		CSManager::active = false;
		Input::active = true;
		Talk::init();
	}
	else if (args[0] == "passthrough")
	{
		if (args[1] == "ent")
		{
			auto srcPath = tr::splitStr(args[2], "-");
			auto tgtPath = tr::splitStr(args[3], "-");
			auto src = World::getLevel(srcPath[0])->getEntity(srcPath[1]);
			auto tgt = World::getLevel(tgtPath[0])->getEntity(tgtPath[1]);
			auto vars = src->getVars();
			for (int i = 0; i < vars.size(); i++)
			{
				tgt->setVar(vars[i].name, vars[i].num);
				tgt->setVar(vars[i].name, vars[i].str);
			}
			tgt->deleteVar("posX");
			tgt->deleteVar("posY");
			tgt->weapon = src->weapon;
			tgt->bauble = src->bauble;
		}
	}
	else if (args[0] == "spawnEnt")
	{
		auto l = World::getCurrentLevel();
		l->ents.push_back(Entity(World::getEntFile(args[1])));
		auto e = &l->ents[l->ents.size() - 1];
		e->setPosition({
			std::stof(args[2].toAnsiString()),
			std::stof(args[3].toAnsiString())
		});
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

void World::saveGame(pugi::xml_node world)
{
	world.append_attribute(L"file") = currentFile.toWideString().c_str();
	world.append_attribute(L"currentLevel") = getCurrentLevel()->name.toWideString().c_str();
	for (int k = 0; k < lvls.size(); k++)
	{
		auto lvl = &lvls[k];
		auto l = world.append_child(L"level");
		l.append_attribute(L"name") = lvl->name.toWideString().c_str();
		auto lvlVars = lvl->getVars();
		for (int i = 0; i < lvlVars.size(); i++)
		{
			auto v = lvlVars[i];
			if (v.str.isEmpty()) l.append_attribute(sf::String(v.name + "_num").toWideString().c_str()) = v.num;
			else l.append_attribute(sf::String(v.name + "_str").toWideString().c_str()) = v.str.toWideString().c_str();
		}
		for (int i = 0; i < lvl->ents.size(); i++)
		{
			auto ent = &lvl->ents[i];
			auto e = l.append_child(L"entity");
			e.append_attribute(L"name") = ent->name.toWideString().c_str();
			auto vars = ent->getVars();
			for (int j = 0; j < vars.size(); j++)
			{
				auto v = vars[j];
				if (v.str.isEmpty()) e.append_attribute(sf::String(v.name + "_num").toWideString().c_str()) = v.num;
				else e.append_attribute(sf::String(v.name + "_str").toWideString().c_str()) = v.str.toWideString().c_str();
			}
		}
		for (int i = 0; i < lvl->triggers.size(); i++)
		{
			auto trig = &lvl->triggers[i];
			auto t = l.append_child(L"trigger");
			auto vars = trig->getVars();
			for (int j = 0; j < vars.size(); j++)
			{
				auto v = vars[j];
				if (v.name == "name") t.append_attribute(L"name") = v.str.toWideString().c_str();
				else if (v.str.isEmpty()) t.append_attribute(sf::String(v.name + "_num").toWideString().c_str()) = v.num;
				else t.append_attribute(sf::String(v.name + "_str").toWideString().c_str()) = v.str.toWideString().c_str();
			}
		}
	}
}

void World::loadGame(pugi::xml_node world)
{
	init();
	loadFromFile(pugi::as_utf8(world.attribute(L"file").as_string()));
	setCurrentLevel(world.attribute(L"currentLevel").as_string());
	for (auto l : world.children(L"level"))
	{
		auto lvl = getLevel(l.attribute(L"name").as_string());
		for (auto data : l.attributes())
		{
			auto name = tr::splitStr(data.name(), "_");
			if (name[1] == "str") { lvl->setVar(name[0], data.as_string()); }
			else if (name[1] == "num") { lvl->setVar(name[0], data.as_float()); }
		}
		for (auto e : l.children(L"entity"))
		{
			auto ent = lvl->getEntity(e.attribute(L"name").as_string());
			for (auto data : e.attributes())
			{
				auto name = tr::splitStr(data.name(), "_");
				if (name[1] == "str") { ent->setVar(name[0], data.as_string()); }
				else if (name[1] == "num") { ent->setVar(name[0], data.as_float()); }
			}
		}
		for (auto t : l.children(L"trigger"))
		{
			auto trig = lvl->getTrigger(t.attribute(L"name").as_string());
			for (auto data : t.attributes())
			{
				auto name = tr::splitStr(data.name(), "_");
				if (name[1] == "str") { trig->setVar(name[0], data.as_string()); }
				else if (name[1] == "num") { trig->setVar(name[0], data.as_float()); }
			}
		}
	}
}