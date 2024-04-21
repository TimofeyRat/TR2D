#include "hCutscene.hpp"
#include "hAssets.hpp"
#include "hWindow.hpp"
#include "hUI.hpp"
#include "hWorld.hpp"
#include "hGlobal.hpp"
#include <pugixml.hpp>

std::vector<CSManager::FrameCutscene> CSManager::frames;
std::vector<CSManager::WorldCutscene> CSManager::worlds;
sf::Vector2i CSManager::current;
bool CSManager::active;
sf::Music CSManager::music;

CSManager::FrameCutscene::Change::Change()
{
	phrase = anim = musicPath = "";
	skippable = false;
}

CSManager::FrameCutscene::FrameCutscene()
{
	name = talkPath = "";
	frame = FrameAnimator();
}

CSManager::FrameCutscene::Change* CSManager::FrameCutscene::getChange(sf::String phrase)
{
	for (int i = 0; i < changes.size(); i++)
	{
		if (changes[i].phrase == phrase)
		{
			return &changes[i];
		}
	}
	return nullptr;
}

void CSManager::init()
{
	active = false;
	frames.clear();
	worlds.clear();
	current = {0, 0};
	for (auto text : AssetManager::getTexts(".trcs"))
	{
		pugi::xml_document file;
		file.load_string(AssetManager::getText(text).toWideString().c_str());
		for (auto cs : file.children())
		{
			if (sf::String(cs.name()) == "frame_cutscene")
			{
				FrameCutscene fc;
				fc.name = cs.attribute(L"name").as_string();
				fc.frame.loadFromFile(sf::String(cs.attribute(L"anim").as_string()));
				fc.talkPath = cs.attribute(L"dialogue").as_string();
				for (auto change : cs.children())
				{
					if (sf::String(change.name()) == "change")
					{
						FrameCutscene::Change c;
						c.phrase = change.attribute(L"phrase").as_string();
						c.skippable = change.attribute(L"skippable").as_bool();
						c.anim = change.attribute(L"anim").as_string();
						c.musicPath = change.attribute(L"music").as_string();
						fc.changes.push_back(c);
					}
				}
				frames.push_back(fc);
			}
			if (sf::String(cs.name()) == "world_cutscene")
			{
				WorldCutscene wc(cs.attribute(L"name").as_string(), cs.attribute(L"dialogue").as_string());
				for (auto clip : cs.children())
				{
					if (sf::String(clip.name()) == "clip")
					{
						WorldCutscene::Change c(
							clip.attribute(L"name").as_string(),
							clip.attribute(L"music").as_string(),
							clip.attribute(L"startPhraseOnEnd").as_bool(),
							clip.attribute(L"duration").as_float()
						);
						for (auto change : clip.children())
						{
							if (sf::String(change.name()) == "move_cam")
							{
								auto ap = change.attribute(L"pos");
								auto as = change.attribute(L"size");
								auto pos = ap.empty() ? std::vector<sf::String>() : tr::splitStr(ap.as_string(), " ");
								auto size = as.empty() ? std::vector<sf::String>() : tr::splitStr(as.as_string(), " ");
								c.cam = WorldCutscene::Change::MoveCam(
									(ap.empty() ? sf::Glsl::Vec4(0, 0, 0, 0) :
									sf::Glsl::Vec4(
										std::stof(pos[0].toAnsiString()), std::stof(pos[1].toAnsiString()),
										std::stof(pos[2].toAnsiString()), std::stof(pos[3].toAnsiString())
									)),
									(as.empty() ? sf::Glsl::Vec4(0, 0, 0, 0) :
									sf::Glsl::Vec4(
										std::stof(size[0].toAnsiString()), std::stof(size[1].toAnsiString()),
										std::stof(size[2].toAnsiString()), std::stof(size[3].toAnsiString())
									)),
									(ap.empty() ? "" : pos[4]), (as.empty() ? "" : size[4])
								);
							}
							else if (sf::String(change.name()) == "move_ent")
							{
								auto sp = tr::splitStr(change.attribute(L"start").as_string(), " ");
								auto ep = tr::splitStr(change.attribute(L"end").as_string(), " ");
								c.moves.push_back(WorldCutscene::Change::MoveEnt(
									change.attribute(L"name").as_string(),
									{std::stof(sp[0].toAnsiString()), std::stof(sp[1].toAnsiString())},
									{std::stof(ep[0].toAnsiString()), std::stof(ep[1].toAnsiString())},
									change.attribute(L"func").as_string()
								));
							}
							else if (sf::String(change.name()) == "anim_ent")
							{
								c.anims.push_back(WorldCutscene::Change::AnimEnt(
									change.attribute(L"name").as_string(),
									change.attribute(L"animation").as_string()
								));
							}
						}
						wc.changes.push_back(c);
					}
				}
				worlds.push_back(wc);
			}
		}
	}
}

void CSManager::setCutscene(sf::String name)
{
	for (int i = 0; i < frames.size(); i++)
	{
		frames[i].frame.restart();
		if (frames[i].name == name) { current = {0, i}; frames[i].frame.setCurrentAnimation(0); }
	}
	for (int i = 0; i < worlds.size(); i++)
	{
		if (worlds[i].name == name) { current = {1, i}; worlds[i].currentChange = worlds[i].changes[0].name; }
	}
}

sf::String CSManager::getTalk()
{
	if (!current.x) { return frames[current.y].talkPath; }
	else { return worlds[current.y].talkPath; }
	return "";
}

sf::String CSManager::getMusic(sf::String phrase)
{
	if (!current.x) { return frames[current.y].getChange(phrase)->musicPath; }
	else { return worlds[current.y].getCurrentChange()->musicPath; }
	return "";
}

CSManager::WorldCutscene::Change::MoveEnt::MoveEnt()
{
	entName = math = "";
	start = target = {0, 0};
}

CSManager::WorldCutscene::Change::MoveEnt::MoveEnt(sf::String name, sf::Vector2f s, sf::Vector2f e, sf::String lerp)
{
	entName = name;
	math = lerp;
	start = s;
	target = e;
}

CSManager::WorldCutscene::Change::MoveCam::MoveCam()
{
	startPos = endPos = startSize = endSize = {0, 0};
	posMath = sizeMath = "";
}

CSManager::WorldCutscene::Change::MoveCam::MoveCam(sf::Glsl::Vec4 pos, sf::Glsl::Vec4 size, sf::String posLerp, sf::String sizeLerp)
{
	startPos = {pos.x, pos.y};
	endPos = {pos.z, pos.w};
	startSize = {size.x, size.y};
	endSize = {size.z, size.w};
	posMath = posLerp;
	sizeMath = sizeLerp;
}

CSManager::WorldCutscene::Change::AnimEnt::AnimEnt()
{
	entName = animName = "";
}

CSManager::WorldCutscene::Change::AnimEnt::AnimEnt(sf::String ent, sf::String anim)
{
	entName = ent;
	animName = anim;
}

CSManager::WorldCutscene::Change::Change()
{
	cam = MoveCam();
	startPhraseOnEnd = false;
	moves.clear();
	anims.clear();
	name = musicPath = "";
	duration = current = 0;
}

CSManager::WorldCutscene::Change::Change(sf::String n, sf::String music, bool spoe, float len)
{
	cam = MoveCam();
	startPhraseOnEnd = spoe;
	moves.clear();
	name = n;
	musicPath = music;
	duration = len;
	current = 0;
}

void CSManager::WorldCutscene::Change::update()
{
	if (current < duration)	current += Window::getDeltaTime();
	auto lvl = World::getCurrentLevel();
	//Update camera
	if (!cam.sizeMath.isEmpty())
	{
		lvl->cam.doUpdate = false;
		sf::Vector2f camSize;
		auto math = tr::splitStr(cam.sizeMath, "-");
		if (math[0] == "linear") { camSize = tr::lerpVec(cam.startSize, cam.endSize, current / duration); }
		else if (math[0] == "lerp") { camSize = tr::lerpVec(lvl->cam.view.getSize(), cam.endSize, Window::getDeltaTime() * std::stof(math[1].toAnsiString())); }
		else if (math[0] == "set") { camSize = cam.endSize; }
		lvl->cam.view.setSize(camSize);
	}
	if (!cam.posMath.isEmpty())
	{
		lvl->cam.doUpdate = false;
		sf::Vector2f camPos;
		auto math = tr::splitStr(cam.posMath, "-");
		if (math[0] == "linear") { camPos = tr::lerpVec(cam.startPos, cam.endPos, current / duration); }
		else if (math[0] == "lerp") { camPos = tr::lerpVec(lvl->cam.view.getCenter(), cam.endPos, Window::getDeltaTime() * std::stof(math[1].toAnsiString())); }
		else if (math[0] == "set") { camPos = cam.endPos; }
		lvl->cam.view.setCenter(camPos);
	}
	for (int i = 0; i < moves.size(); i++)
	{
		auto e = moves[i];
		auto ent = lvl->getEntity(e.entName);
		ent->getRigidbody()->getBody()->SetLinearVelocity({0, 0});
		auto math = tr::splitStr(e.math, "-");
		if (math[0] == "linear")
		{
			auto d = (math.size() == 2 ? std::stof(math[1].toAnsiString()) : duration);
			ent->setPosition(tr::lerpVec(e.start, e.target, tr::clamp(current, 0, d) / d));
		}
		else if (math[0] == "move" && current < duration)
		{
			ent->setPosition(tr::lerpVec(e.start, e.target, current / duration));
			auto pos = ent->getRigidbody()->getPosition();
			auto vel = tr::clampVec(e.target - sf::Vector2f(pos.x, pos.y), {-1, -1}, {1, 1});
			ent->setVar("dx", (int)vel.x);
			ent->setVar("dy", (int)vel.y);
			ent->getRigidbody()->getBody()->SetLinearVelocity({(int)vel.x, (int)vel.y});
		}
		else if (math[0] == "set") { ent->setPosition(e.target); }
	}
	for (int i = 0; i < anims.size(); i++)
	{
		auto a = anims[i];
		auto ent = lvl->getEntity(a.entName);
		ent->setVar("dontUpdateAnim", 1);
		ent->setVar("anim", a.animName);
	}
}

CSManager::WorldCutscene::WorldCutscene()
{
	name = talkPath = currentChange = "";
	changes.clear();
}

CSManager::WorldCutscene::WorldCutscene(sf::String n, sf::String talk)
{
	name = n;
	talkPath = talk;
	currentChange = "";
	changes.clear();
}

CSManager::WorldCutscene::Change *CSManager::WorldCutscene::getChange(sf::String n)
{
	for (int i = 0; i < changes.size(); i++)
	{
		if (changes[i].name == n) return &changes[i];
	}
	return nullptr;
}

CSManager::WorldCutscene::Change *CSManager::WorldCutscene::getCurrentChange()
{
	return getChange(currentChange);
}

void CSManager::WorldCutscene::update()
{
	if (currentChange.isEmpty()) currentChange = changes[0].name;
	getCurrentChange()->update();
}