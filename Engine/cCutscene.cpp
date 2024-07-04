#include "hCutscene.hpp"
#include "hAssets.hpp"
#include "hWindow.hpp"
#include "hUI.hpp"
#include "hWorld.hpp"
#include "hGlobal.hpp"
#include <pugixml.hpp>
#include "hInput.hpp"

std::vector<CSManager::FrameCutscene> CSManager::frames;
std::vector<CSManager::WorldCutscene> CSManager::worlds;
sf::Vector2i CSManager::current;
bool CSManager::active, CSManager::shouldEnd;
sf::Music CSManager::music;
sf::String CSManager::currentMusic;

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
	music.setVolume(0);
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
						c.waitAll = clip.attribute(L"waitAll").as_bool();
						for (auto change : clip.children())
						{
							if (sf::String(change.name()) == "move_cam")
							{
								auto pts = tr::splitStr(change.attribute(L"points").as_string(), "|");
								std::vector<sf::Vector2f> curve;
								for (int i = 0; i < pts.size(); i++)
								{
									auto p = tr::splitStr(pts[i], " ");
									curve.push_back({
										std::stof(p[0].toAnsiString()),
										std::stof(p[1].toAnsiString())
									});
								}
								auto as = change.attribute(L"size");
								auto size = as.empty() ? std::vector<sf::String>() : tr::splitStr(as.as_string(), " ");
								WorldCutscene::Change::MoveCam mc;
								mc.pos = curve;
								mc.startSize = {std::stof(size[0].toAnsiString()), std::stof(size[1].toAnsiString())};
								mc.endSize = {std::stof(size[2].toAnsiString()), std::stof(size[3].toAnsiString())};
								mc.sizeMath = (as.empty() ? "" : size[4]);
								mc.resetBasis(
									change.attribute(L"offset").as_float(),
									change.attribute(L"duration").as_float(c.duration),
									change.attribute(L"continueAfterEnd").as_bool(true)
								);
								c.cam.push_back(mc);
							}
							else if (sf::String(change.name()) == "move_ent")
							{
								auto pts = tr::splitStr(change.attribute(L"points").as_string(), "|");
								std::vector<sf::Vector2f> curve;
								for (int i = 0; i < pts.size(); i++)
								{
									auto p = tr::splitStr(pts[i], " ");
									curve.push_back({
										(p[0] == "current" ? UINT64_MAX : std::stof(p[0].toAnsiString())),
										(p[1] == "current" ? UINT64_MAX : std::stof(p[1].toAnsiString()))
									});
								}
								WorldCutscene::Change::MoveEnt me;
								me.entName = change.attribute(L"name").as_string();
								me.curve = curve;
								me.resetBasis(
									change.attribute(L"offset").as_float(),
									change.attribute(L"duration").as_float(c.duration),
									change.attribute(L"continueAfterEnd").as_bool(true)
								);
								c.moves.push_back(me);
							}
							else if (sf::String(change.name()) == "anim_ent")
							{
								auto soe = change.attribute(L"stopOnEnd");
								WorldCutscene::Change::AnimEnt ae;
								ae.entName = change.attribute(L"name").as_string();
								ae.animName = change.attribute(L"animation").as_string();
								ae.animSpeed = change.attribute(L"animSpeed").as_float(1);
								ae.resetBasis(
									change.attribute(L"offset").as_float(),
									change.attribute(L"duration").as_float(c.duration),
									change.attribute(L"continueAfterEnd").as_bool(true)
								);
								c.anims.push_back(ae);
							}
							else if (sf::String(change.name()) == "execute")
							{
								WorldCutscene::Change::Execute e;
								e.command = change.text().as_string();
								e.count = change.attribute(L"count").as_int();
								e.freq = change.attribute(L"frequency").as_float();
								e.resetBasis(
									change.attribute(L"offset").as_float(),
									change.attribute(L"duration").as_float(c.duration),
									change.attribute(L"continueAfterEnd").as_bool(true)
								);
								c.exec.push_back(e);
							}
							else if (sf::String(change.name()) == "curve_ents")
							{
								WorldCutscene::Change::CurveEnts ce;
								ce.entA = change.attribute(L"entA").as_string();
								ce.entB = change.attribute(L"entB").as_string();
								ce.created = false;
								auto pts = tr::splitStr(change.attribute(L"points").as_string(), "|");
								for (int i = 0; i < pts.size(); i++)
								{
									auto p = tr::splitStr(pts[i], " ");
									ce.curve.push_back({
										std::stof(p[0].toAnsiString()),
										std::stof(p[1].toAnsiString())
									});
								}
								ce.count = change.attribute(L"count").as_int();
								ce.partName = change.attribute(L"particle").as_string();
								auto s = tr::splitStr(change.attribute(L"partSpeed").as_string(), " ");
								ce.speed = {
									std::stof(s[0].toAnsiString()), std::stof(s[1].toAnsiString()),
									std::stof(s[2].toAnsiString()), std::stof(s[3].toAnsiString())
								};
								ce.lt = change.attribute(L"lifeTime").as_float();
								ce.s = change.attribute(L"effectSpeed").as_float();
								ce.l = change.attribute(L"effectLength").as_float();
								ce.math = change.attribute(L"effectType").as_string();
								ce.resetBasis(
									change.attribute(L"offset").as_float(),
									change.attribute(L"duration").as_float(c.duration),
									change.attribute(L"continueAfterEnd").as_bool(true)
								);
								c.cents.push_back(ce);
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
	if (!current.x && frames.size() && current.y == tr::clamp(current.y, 0, frames.size() - 1))
	{
		if (auto c = frames[current.y].getChange(phrase)) return c->musicPath;
	}
	else if (worlds.size())
	{
		if (auto c = worlds[current.y].getCurrentChange()) return c->musicPath;
	}
	return "";
}

CSManager::WorldCutscene::Change::Basis::Basis(float os, float d, bool cae)
{
	resetBasis(os, d, cae);
	ended = false;
}

void CSManager::WorldCutscene::Change::Basis::basisUpdate()
{
	if (offset != 0)
	{
		ot += Window::getDeltaTime();
		if (ot < offset) return;
	}
	if (current < duration) current += Window::getDeltaTime();
	else { current = duration; ended = true; }
}

bool CSManager::WorldCutscene::Change::Basis::isActive()
{
	if (offset != 0 && ot < offset) { return false; }
	if (duration != 0 && current >= duration) { return continueAfterEnd; }
	return true;
}

void CSManager::WorldCutscene::Change::Basis::resetBasis(float os, float d, bool cae)
{
	offset = os;
	ot = 0;
	duration = d;
	current = 0;
	continueAfterEnd = cae;
}

CSManager::WorldCutscene::Change::MoveEnt::MoveEnt() : Basis()
{
	entName = "";
	curve.clear();
}

void CSManager::WorldCutscene::Change::MoveEnt::update()
{
	basisUpdate();
	if (!isActive()) return;
	auto ent = World::getCurrentLevel()->getEntity(entName);
	ent->getRigidbody()->getBody()->SetLinearVelocity({0, 0});
	if (curve[0].x == UINT64_MAX) curve[0].x = (int)ent->getPosition().x;
	if (curve[0].y == UINT64_MAX) curve[0].y = (int)ent->getPosition().y;
	ent->setPosition(tr::getBezierPoint(curve, current / duration));
}

CSManager::WorldCutscene::Change::MoveCam::MoveCam() : Basis()
{
	pos.clear();
	startSize = endSize = {0, 0};
	sizeMath = "";
}

void CSManager::WorldCutscene::Change::MoveCam::update()
{
	basisUpdate();
	if (!isActive()) return;
	auto c = &World::getCurrentLevel()->cam;
	c->doUpdate = false;
	if (!sizeMath.isEmpty())
	{
		sf::Vector2f size;
		auto math = tr::splitStr(sizeMath, "-");
		if (math[0] == "linear") { size = tr::lerpVec(startSize, endSize, current / duration); }
		else if (math[0] == "lerp") { size = tr::lerpVec(c->view.getSize(), endSize, Window::getDeltaTime() * std::stof(math[1].toAnsiString())); }
		else if (math[0] == "set") { size = endSize; }
		c->view.setSize(size);
	}
	if (!pos.empty())
	{
		c->view.setCenter(tr::getBezierPoint(pos, current / duration));
	}
}

CSManager::WorldCutscene::Change::AnimEnt::AnimEnt() : Basis()
{
	entName = animName = "";
	animSpeed = 0;
}

void CSManager::WorldCutscene::Change::AnimEnt::update()
{
	basisUpdate();
	auto e = World::getCurrentLevel()->getEntity(entName);
	if (!isActive()) return;
	e->setVar("dontUpdateAnim", 1);
	e->setVar("anim", animName);
	e->getSkeleton()->setSpeed(animSpeed);
}

CSManager::WorldCutscene::Change::Execute::Execute() : Basis()
{
	command = "";
	count = executed = timer = freq = 0;
}

void CSManager::WorldCutscene::Change::Execute::update()
{
	basisUpdate();
	if (!isActive()) return;
	if (freq != 0)
	{
		if (timer < freq) timer += Window::getDeltaTime();
		else { if (count != 0) { if (executed < count) { executed++; execute(); } } else { execute(); } timer = 0; }
	}
	else if (count != 0) { if (executed < count) { executed++; execute(); } }
	else execute();
}

void CSManager::WorldCutscene::Change::Execute::execute()
{
	auto c = tr::splitStr(command, ";");
	for (int j = 0; j < c.size(); j++) tr::execute(c[j]);
}

CSManager::WorldCutscene::Change::CurveEnts::CurveEnts() : Basis()
{
	created = false;
	curve.clear();
	count = 0;
	partName = math = "";
	speed = {0, 0, 0, 0};
	lt = s = l = 0;
}

void CSManager::WorldCutscene::Change::CurveEnts::update()
{
	basisUpdate();
	if (!isActive() || created) return;
	World::ParticleCurve pc;
	if (math == "set") pc.math = World::ParticleCurve::Set;
	if (math == "fadeIn") pc.math = World::ParticleCurve::FadeIn;
	if (math == "lerp") pc.math = World::ParticleCurve::FadeLerp;
	if (math == "pulse") pc.math = World::ParticleCurve::Pulse;
	pc.init(World::getCurrentLevel()->world, World::ParticleCurve::Type::TwoEnts, entA, entB, curve, count, partName, speed, lt, s, l, duration);
	World::getCurrentLevel()->partCurves.push_back(pc);
	created = true;
}

CSManager::WorldCutscene::Change::Change()
{
	startPhraseOnEnd = false;
	cam.clear();
	moves.clear();
	anims.clear();
	exec.clear();
	cents.clear();
	name = musicPath = "";
	duration = current = 0;
}

CSManager::WorldCutscene::Change::Change(sf::String n, sf::String music, bool spoe, float len)
{
	startPhraseOnEnd = spoe;
	cam.clear();
	moves.clear();
	anims.clear();
	exec.clear();
	cents.clear();
	name = n;
	musicPath = music;
	duration = len;
	current = 0;
}

void CSManager::WorldCutscene::Change::update()
{
	if (current < duration)	current += Window::getDeltaTime();
	auto lvl = World::getCurrentLevel();
	for (int i = 0; i < cam.size(); i++) cam[i].update();
	for (int i = 0; i < moves.size(); i++) moves[i].update();
	for (int i = 0; i < anims.size(); i++) anims[i].update();
	for (int i = 0; i < exec.size(); i++) exec[i].update();
	for (int i = 0; i < cents.size(); i++) cents[i].update();
	if (shouldEnd)
	{
		if (!waitAll) { shouldEnd = false; active = false; return; }
		bool end = true;
		for (int i = 0; i < cam.size(); i++) { if (!cam[i].ended) { end = false; break; } }
		for (int i = 0; i < moves.size(); i++) { if (!moves[i].ended) { end = false; break; } }
		for (int i = 0; i < anims.size(); i++) { if (!anims[i].ended) { end = false; break; } }
		for (int i = 0; i < exec.size(); i++) { if (!anims[i].ended) { end = false; break; } }
		for (int i = 0; i < cents.size(); i++) { if (!cents[i].ended) { end = false; break; } }
		if (end) { shouldEnd = false; active = false; }
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
		for (int i = 0; i < c->cam.size(); i++)
		{
			target->draw(tr::generateBezier(c->cam[i].pos, 0.1, sf::Color::Blue));
		}
		for (int i = 0; i < c->moves.size(); i++)
		{
			target->draw(tr::generateBezier(c->moves[i].curve, 0.1, sf::Color::Red));
		}
	}
}