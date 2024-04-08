#include "hCutscene.hpp"
#include "hAssets.hpp"
#include "hWindow.hpp"
#include "hUI.hpp"

#include <pugixml.hpp>

std::vector<CSManager::FrameCutscene> CSManager::frames;
sf::Vector2i CSManager::current;
bool CSManager::active;

CSManager::FrameCutscene::Change::Change()
{
	phrase = anim = "";
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
						fc.changes.push_back(c);
					}
				}
				frames.push_back(fc);
			}
		}
	}
}

void CSManager::setCutscene(sf::String name)
{
	for (int i = 0; i < frames.size(); i++)
	{
		frames[i].frame.restart();
		if (frames[i].name == name) { current = {0, i}; }
	}
}

sf::String CSManager::getTalk()
{
	if (!current.x)
	{
		return frames[current.y].talkPath;
	}
	return "";
}