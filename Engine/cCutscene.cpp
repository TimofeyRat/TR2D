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
								auto pts = tr::splitStr(change.attribute(L"points").as_string(), "|");
								std::vector<sf::Vector2f> curve;
								for (int i = 0; i < pts.size() - 1; i++)
								{
									auto p = tr::splitStr(pts[i], " ");
									curve.push_back({
										std::stof(p[0].toAnsiString()),
										std::stof(p[1].toAnsiString())
									});
								}
								auto as = change.attribute(L"size");
								auto size = as.empty() ? std::vector<sf::String>() : tr::splitStr(as.as_string(), " ");
								c.cam = WorldCutscene::Change::MoveCam(
									curve,
									(as.empty() ? sf::Glsl::Vec4(0, 0, 0, 0) :
									sf::Glsl::Vec4(
										std::stof(size[0].toAnsiString()), std::stof(size[1].toAnsiString()),
										std::stof(size[2].toAnsiString()), std::stof(size[3].toAnsiString())
									)),
									(as.empty() ? "" : size[4]), std::stof(pts[pts.size()- 1].toAnsiString())
								);
								c.cam.offset = change.attribute(L"offset").as_float();
							}
							else if (sf::String(change.name()) == "move_ent")
							{
								auto pts = tr::splitStr(change.attribute(L"points").as_string(), "|");
								std::vector<sf::Vector2f> curve;
								for (int i = 0; i < pts.size() - 1; i++)
								{
									auto p = tr::splitStr(pts[i], " ");
									curve.push_back({
										std::stof(p[0].toAnsiString()),
										std::stof(p[1].toAnsiString())
									});
								}
								auto m = WorldCutscene::Change::MoveEnt(
									change.attribute(L"name").as_string(),
									curve,
									std::stof(pts[pts.size() - 1].toAnsiString())
								);
								m.offset = change.attribute(L"offset").as_float();
								c.moves.push_back(m);
							}
							else if (sf::String(change.name()) == "anim_ent")
							{
								auto soe = change.attribute(L"stopOnEnd");
								auto a = WorldCutscene::Change::AnimEnt(
									change.attribute(L"name").as_string(),
									change.attribute(L"animation").as_string(),
									(soe.empty() ? false : soe.as_bool())
								);
								a.offset = change.attribute(L"offset").as_float();
								c.anims.push_back(a);
							}
							else if (sf::String(change.name()) == "execute")
							{
								auto e = WorldCutscene::Change::Execute(
									change.text().get(),
									change.attribute(L"count").as_int(),
									change.attribute(L"freq").as_float()
								);
								e.offset = change.attribute(L"offset").as_float();
								c.exec.push_back(e);
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
	entName = "";
	duration = 0;
	curve.clear();
}

CSManager::WorldCutscene::Change::MoveEnt::MoveEnt(sf::String name, std::vector<sf::Vector2f> pts, float lerp)
{
	entName = name;
	duration = lerp;
	curve = pts;
}

CSManager::WorldCutscene::Change::MoveCam::MoveCam()
{
	curve.clear();
	startSize = endSize = {0, 0};
	sizeMath = "";
	posDuration = 0;
}

CSManager::WorldCutscene::Change::MoveCam::MoveCam(std::vector<sf::Vector2f> pts, sf::Glsl::Vec4 size, sf::String sizeLerp, float ps)
{
	curve = pts;
	startSize = {size.x, size.y};
	endSize = {size.z, size.w};
	sizeMath = sizeLerp;
	posDuration = ps;
}

CSManager::WorldCutscene::Change::AnimEnt::AnimEnt()
{
	entName = animName = "";
}

CSManager::WorldCutscene::Change::AnimEnt::AnimEnt(sf::String ent, sf::String anim, bool soe)
{
	entName = ent;
	animName = anim;
	stopOnEnd = soe;
}

CSManager::WorldCutscene::Change::Execute::Execute()
{
	command = "";
	freq = count = exec = timer = 0;
}

CSManager::WorldCutscene::Change::Execute::Execute(sf::String cmd, int c, float frequency)
{
	command = cmd;
	freq = frequency;
	count = c;
	exec = timer = 0;
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
	if (!cam.sizeMath.isEmpty() && cam.offset <= 0)
	{
		lvl->cam.doUpdate = false;
		sf::Vector2f camSize;
		auto math = tr::splitStr(cam.sizeMath, "-");
		if (math[0] == "linear") { camSize = tr::lerpVec(cam.startSize, cam.endSize, current / duration); }
		else if (math[0] == "lerp") { camSize = tr::lerpVec(lvl->cam.view.getSize(), cam.endSize, Window::getDeltaTime() * std::stof(math[1].toAnsiString())); }
		else if (math[0] == "set") { camSize = cam.endSize; }
		lvl->cam.view.setSize(camSize);
	}
	if (cam.posDuration != 0 && cam.offset <= 0)
	{
		lvl->cam.doUpdate = false;
		lvl->cam.view.setCenter(tr::getBezierPoint(cam.curve, tr::clamp(current / cam.posDuration, 0, 1)));
	}
	cam.offset -= Window::getDeltaTime();
	for (int i = 0; i < moves.size(); i++)
	{
		auto e = moves[i];
		e.offset -= Window::getDeltaTime();
		if (e.offset > 0) continue;
		auto ent = lvl->getEntity(e.entName);
		ent->getRigidbody()->getBody()->SetLinearVelocity({0, 0});
		ent->setPosition(tr::getBezierPoint(e.curve, tr::clamp(current / e.duration, 0, 1)));
	}
	for (int i = 0; i < anims.size(); i++)
	{
		auto a = anims[i];
		a.offset -= Window::getDeltaTime();
		if (a.offset > 0) continue;
		auto ent = lvl->getEntity(a.entName);
		if (a.stopOnEnd && current >= duration) { ent->setVar("dontUpdateAnim", 0); continue; }
		ent->setVar("dontUpdateAnim", 1);
		auto anim = tr::splitStr(a.animName, " ");
		ent->setVar("anim", anim[0]);
		ent->getSkeleton()->setSpeed(anim.size() == 2 ? std::stof(anim[1].toAnsiString()) : 1);
	}
	for (int i = 0; i < exec.size(); i++)
	{
		auto e = &exec[i];
		e->offset -= Window::getDeltaTime();
		if (e->offset > 0) continue;
		if (e->exec >= e->count) continue;
		if (e->freq != 0)
		{
			if (e->timer >= e->freq)
			{
				auto c = tr::splitStr(e->command, ";");
				for (int j = 0; j < c.size(); j++) tr::execute(c[j]);
				e->timer = 0;
				e->exec++;
			}
			e->timer += Window::getDeltaTime();
		}
		else
		{
			e->exec++;
			auto c = tr::splitStr(e->command, ";");
			for (int j = 0; j < c.size(); j++) tr::execute(c[j]);
		}
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

void CSManager::drawDebug(sf::RenderTarget *target)
{
	if (current.x)
	{
		auto c = worlds[current.y].getCurrentChange();
		target->draw(tr::generateBezier(c->cam.curve, 0.1, sf::Color::Blue));
		for (int i = 0; i < c->moves.size(); i++)
		{
			target->draw(tr::generateBezier(c->moves[i].curve, 0.1, sf::Color::Red));
		}
	}
}