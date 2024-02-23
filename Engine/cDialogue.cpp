#include "hDialogue.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hProgrammable.hpp"
#include "hWorld.hpp"
#include "hWindow.hpp"

#define PUGIXML_WCHAR_MODE
#include <pugixml.hpp>

#include <iostream>

bool Talk::active;
sf::String Talk::currentDialogue;
std::vector<Talk::Dialogue> Talk::dialogues;

Talk::Dialogue::Phrase::Reply::Reply()
{
	actions.clear();
	name = text = condition = "";
}
Talk::Dialogue::Phrase::Reply::Reply(sf::String N, sf::String T, sf::String C)
{
	actions.clear();
	name = N;
	text = T;
	condition = C;
}

Talk::Dialogue::Phrase::Phrase()
{
	replies.clear();
	name = text = condition = "";
}
Talk::Dialogue::Phrase::Phrase(sf::String N, sf::String T, sf::String C)
{
	replies.clear();
	name = N;
	text = T;
	condition = C;
}

Talk::Dialogue::Dialogue()
{
	phrases.clear();
	name = "";
}
Talk::Dialogue::Dialogue(sf::String N)
{
	phrases.clear();
	name = N;
}

Talk::Dialogue::Phrase *Talk::Dialogue::getCurrentPhrase()
{
	for (int i = 0; i < phrases.size(); i++)
	{
		if (phrases[i].name == currentPhrase)
		{
			return &phrases[i];
		}
	}
}

void Talk::init()
{
	dialogues.clear();
	currentDialogue = "";
	active = false;
}

//Fix the cyrillic

void Talk::loadFromFile(std::string filename)
{
	init();
	pugi::xml_document document;
	document.load_string(AssetManager::getText(filename).toWideString().c_str());
	// document.load_file(filename.c_str());
	for (auto dialogue = document.first_child(); dialogue != pugi::xml_node(); dialogue = dialogue.next_sibling())
	{
		if (sf::String(dialogue.name()) != "dialogue") { continue; }
		Dialogue d(dialogue.attribute(L"name").as_string());
		for (auto phrase = dialogue.first_child(); phrase != pugi::xml_node(); phrase = phrase.next_sibling())
		{
			if (sf::String(phrase.name()) != "phrase") { continue; }
			Dialogue::Phrase p(
				phrase.attribute(L"name").as_string(),
				phrase.child_value(L"text"),
				phrase.attribute(L"condition").as_string()
			);
			for (auto reply = phrase.first_child(); reply != pugi::xml_node(); reply = reply.next_sibling())
			{
				if (sf::String(reply.name()) != "reply") { continue; }
				auto r = Dialogue::Phrase::Reply(
					reply.attribute(L"name").as_string(),
					reply.child_value(L"text"),
					reply.attribute(L"condition").as_string()
				);
				for (auto action = reply.first_child(); action != pugi::xml_node(); action = action.next_sibling())
				{
					if (sf::String(action.name()) != "action") { continue; }
					r.actions.push_back(action.text().get());
				}
				p.replies.push_back(r);
			}
			d.phrases.push_back(p);
		}
		d.currentPhrase = d.phrases[0].name;
		dialogues.push_back(d);
	}
	currentDialogue = dialogues[0].name;
}

Talk::Dialogue *Talk::getCurrentDialogue()
{
	for (int i = 0; i < dialogues.size(); i++)
	{
		if (dialogues[i].name == currentDialogue)
		{
			return &dialogues[i];
		}
	}
}

bool Talk::conditionCheck(sf::String condition)
{
	bool activate = true;
	auto rules = tr::splitStr(condition, ";");
	for (int i = 0; i < rules.size(); i++)
	{
		auto args = tr::splitStr(rules[i], "=");
		auto owner = tr::splitStr(args[0], "-");
		Programmable::Variable var;
		sf::String v = args[1], cmp = args[2], val = args[3];
		if (owner[0] == "ent") { var = World::getEnt(owner[1])->getVar(owner[2]); }
		else if (owner[0] == "Window") { var = Window::getVar(owner[1]); }
		bool numOrStr = false;
		if (v == "num") { numOrStr = false; v = std::to_string(var); }
		else if (v == "str") { numOrStr = true; v = var; }
		if (cmp == "lt" && !(std::stof(v.toAnsiString()) < std::stof(val.toAnsiString()))) { activate = false; continue; }
		if (cmp == "le" && !(std::stof(v.toAnsiString()) <= std::stof(val.toAnsiString()))) { activate = false; continue; }
		if (cmp == "ge" && !(std::stof(v.toAnsiString()) >= std::stof(val.toAnsiString()))) { activate = false; continue; }
		if (cmp == "gt" && !(std::stof(v.toAnsiString()) > std::stof(val.toAnsiString()))) { activate = false; continue; }
		if (cmp == "e")
		{
			if (numOrStr) { if (!(v == val)) { activate = false; continue; } }
			else { if (!(std::stof(v.toAnsiString()) == std::stof(val.toAnsiString()))) { activate = false; continue; } }
		}
		if (cmp == "ne")
		{
			if (numOrStr) { if (!(v != val)) { activate = false; continue; } }
			else { if (!(std::stof(v.toAnsiString()) != std::stof(val.toAnsiString()))) { activate = false; continue; } }
		}
	}
	return activate;
}

void Talk::restart()
{
	currentDialogue = dialogues[0].name;
	for (int i = 0; i < dialogues.size(); i++)
	{
		dialogues[i].currentPhrase = dialogues[i].phrases[0].name;
	}
}