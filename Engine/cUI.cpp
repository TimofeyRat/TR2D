#include "hUI.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hInput.hpp"
#include "hInventory.hpp"
#include "hWorld.hpp"
#include "hDialogue.hpp"
#include "hAssets.hpp"
#include "hCutscene.hpp"

#include <iostream>
#include <filesystem>

sf::String UI::currentFrame, UI::currentFile;
std::vector<UI::Frame> UI::frames;
bool UI::reloadUI;

UI::Frame::Object::Sprite::Sprite()
{
	active = false;
	spr = sf::Sprite();
	fa = FrameAnimator();
	clear();
}

UI::Frame::Object::Sprite::Sprite(pugi::xml_node node)
{
	parse(node);
}

void UI::Frame::Object::Sprite::update()
{
	active = updateToggle(active, getVar("toggle"), spr.getGlobalBounds());
	if (fa.getCurrentAnim())
	{
		fa.setCurrentAnimation(active ? getVar("active").str : getVar("idle").str);
		fa.update();
		fa.send(spr, false, false);
	}
	spr.setPosition(getObjectPosition(getVar("position"), active, spr.getPosition()));
	spr.setOrigin(getObjectOrigin((sf::Vector2f)spr.getTextureRect().getSize(), getVar("origin"), active, spr.getOrigin()));
	spr.setScale(getObjectScale(
		(sf::Vector2f)spr.getTextureRect().getSize(),
		getVar("scale"),
		active,
		spr.getScale()
	));
	for (int i = 0; i < actions.size(); i++)
	{
		if (actions[i].isAvailable(active) && actions[i].isActivated()) { actions[i].execute(); }
	}
}

void UI::Frame::Object::Sprite::parse(pugi::xml_node node)
{
	setVar("name", node.attribute(L"name").as_string());
	for (auto part : node.children())
	{
		auto name = sf::String(part.name());
		if (name == "texture")
		{
			fa.loadFromFile(sf::String(part.attribute(L"path").as_string()).toAnsiString());
			setVar("active", part.attribute(L"activeAnim").as_string());
			setVar("idle", part.attribute(L"idleAnim").as_string());
			fa.setCurrentAnimation(getVar("idle").str);
		}
		else if (name == "toggle") { setVar("toggle", part.attribute(L"event").as_string()); }
		else if (name == "action")
		{
			actions.push_back(Frame::Object::Action(
				part.attribute(L"event").as_string(),
				part.attribute(L"condition").as_string(),
				part.text().get()
			));
		}
		else if (name == "transparent") { spr.setColor({0, 0, 0, 0}); }
		else
		{
			setVar(part.name(),
				sf::String(part.attribute(L"type").as_string()) +
				" " + part.text().as_string()
			);
		}
	}
}

UI::Frame::Object::Text::Text()
{
	active = false;
	txt = sf::Text();
	font = nullptr;
	activeClr = idleClr = activeOut = idleOut = {0, 0, 0, 0};
	curClr = {0, 0, 0, 0};
	clear();
}

UI::Frame::Object::Text::Text(pugi::xml_node node)
{
	parse(node);
}

void UI::Frame::Object::Text::update()
{
	active = updateToggle(active, getVar("toggle"), txt.getGlobalBounds());
	txt.setFont(*font);
	updateText();
	updateClr();
	updateSize();
	txt.setPosition(getObjectPosition(getVar("position"), active, txt.getPosition()));
	txt.setOrigin(getObjectOrigin(txt.getGlobalBounds().getSize(), getVar("origin"), active, txt.getOrigin()));
	txt.scale(getObjectScale(txt.getGlobalBounds().getSize(), getVar("scale"), active, txt.getScale()));
	for (int i = 0; i < actions.size(); i++)
	{
		if (actions[i].isAvailable(active) && actions[i].isActivated()) { actions[i].execute(); }
	}
}

void UI::Frame::Object::Text::updateSize()
{
	float size = active ? getVar("activeSize") : getVar("idleSize");
	float out = active ? getVar("activeOut") : getVar("idleOut");
	float curSize = getVar("curSize"), curOut = getVar("curOut");
	auto sc = getVar("sizeChange").str;
	if (sc == "set") { curSize = size; curOut = out; }
	else if (sc == "lerp")
	{
		curSize = tr::lerp(curSize, size, 5 * Window::getDeltaTime());
		curOut = tr::lerp(curOut, out, 5 * Window::getDeltaTime());
	}
	setVar("curSize", curSize); setVar("curOut", curOut);
	txt.setCharacterSize(curSize); txt.setOutlineThickness(curOut);
}

void UI::Frame::Object::Text::updateClr()
{
	if (active)
	{
		auto clr = getVar("activeClrType").str;
		auto out = getVar("activeOutType").str;
		if (clr == "set") { curClr = activeClr; }
		else if (clr == "lerp") { curClr = tr::lerpClr(curClr, activeClr, 5 * Window::getDeltaTime()); }
		if (out == "set") { curOut = activeOut; }
		else if (out == "lerp") { curOut = tr::lerpClr(curOut, activeOut, 5 * Window::getDeltaTime()); }
	}
	else
	{
		auto clr = getVar("idleClrType").str;
		auto out = getVar("idleOutType").str;
		if (clr == "set") { curClr = idleClr; }
		else if (clr == "lerp") { curClr = tr::lerpClr(curClr, idleClr, 5 * Window::getDeltaTime()); }
		if (out == "set") { curOut = idleOut; }
		else if (out == "lerp") { curOut = tr::lerpClr(curOut, idleOut, 5 * Window::getDeltaTime()); }
	}
	txt.setFillColor({
		tr::clamp(curClr.x, 0, 255),
		tr::clamp(curClr.y, 0, 255),
		tr::clamp(curClr.z, 0, 255),
		tr::clamp(curClr.w, 0, 255)
	});
	txt.setOutlineColor({
		tr::clamp(curOut.x, 0, 255),
		tr::clamp(curOut.y, 0, 255),
		tr::clamp(curOut.z, 0, 255),
		tr::clamp(curOut.w, 0, 255)
	});
}

void UI::Frame::Object::Text::updateText()
{
	auto text = active ?
	getText(
			getVar("activeAppear").str,
			getVar("activeTyping")
		) :
	getText(
		getVar("idleAppear").str,
		getVar("idleTyping")
	);
	txt.setString(text);
}

sf::String UI::Frame::Object::Text::getText(sf::String appear, float speed)
{
	if (active)
	{
		setVar("icc", 0);
		if (appear == "set") { return parseText(activeTxt); }
		else if (appear == "typing")
		{
			auto cc = getVar("acc") + Window::getDeltaTime() * speed;
			if (cc < 0) cc = 0;
			setVar("acc", cc);
			if (cc == 0) return "";
			return parseText(activeTxt).substring(0, cc);
		}
	}
	else
	{
		setVar("acc", 1);
		if (appear == "set") { return parseText(idleTxt); }
		else if (appear == "typing")
		{
			auto cc = getVar("icc") + Window::getDeltaTime() * speed;
			if (cc < 0) cc = 0;
			setVar("icc", cc);
			if (cc == 0) return "";
			return parseText(idleTxt).substring(0, cc);
		}
	}
	return "";
}

void UI::Frame::Object::Text::parse(pugi::xml_node node)
{
	setVar("name", node.attribute(L"name").as_string());
	for (auto part : node.children())
	{
		auto name = sf::String(part.name());
		if (name == "font")
		{
			font = AssetManager::getFont(pugi::as_utf8(part.attribute(L"path").as_string()));
			setVar("idleSize", part.attribute(L"idleSize").as_int());
			setVar("activeSize", part.attribute(L"activeSize").as_int());
			setVar("idleOut", part.attribute(L"idleOutline").as_int());
			setVar("activeOut", part.attribute(L"activeOutline").as_int());
			setVar("sizeChange", part.attribute(L"sizeChange").as_string());
		}
		else if (name == "txt")
		{
			auto type = sf::String(part.attribute(L"type").as_string());
			if (type == "active") { activeTxt = part.text().as_string(); }
			else if (type == "idle") { idleTxt = part.text().as_string(); }
			auto appear = tr::splitStr(sf::String(part.attribute(L"appear").as_string()), "-");
			if (tr::strContains(appear[0], "typing"))
			{
				setVar(type + "Typing", std::stof(appear[1].toAnsiString()));
			}
			setVar(type + "Appear", appear[0]);
		}
		else if (name == "color")
		{
			auto type = sf::String(part.attribute(L"state").as_string());
			auto clr = tr::splitStr(part.text().as_string(), " ");
			if (type == "active")
			{
				activeClr = {
					std::stof(clr[0].toAnsiString()), std::stof(clr[1].toAnsiString()),
					std::stof(clr[2].toAnsiString()), std::stof(clr[3].toAnsiString())
				};
			}
			else if (type == "idle")
			{
				idleClr = {
					std::stof(clr[0].toAnsiString()), std::stof(clr[1].toAnsiString()),
					std::stof(clr[2].toAnsiString()), std::stof(clr[3].toAnsiString())
				};
			}
			setVar(type + "ClrType", part.attribute(L"type").as_string());
		}
		else if (name == "outline")
		{
			auto type = sf::String(part.attribute(L"state").as_string());
			auto clr = tr::splitStr(part.text().as_string(), " ");
			if (type == "active")
			{
				activeOut = {
					std::stof(clr[0].toAnsiString()), std::stof(clr[1].toAnsiString()),
					std::stof(clr[2].toAnsiString()), std::stof(clr[3].toAnsiString())
				};
			}
			else if (type == "idle")
			{
				idleOut = {
					std::stof(clr[0].toAnsiString()), std::stof(clr[1].toAnsiString()),
					std::stof(clr[2].toAnsiString()), std::stof(clr[3].toAnsiString())
				};
			}
			setVar(type + "OutType", part.attribute(L"type").as_string());
		}
		else if (name == "toggle") { setVar("toggle", part.attribute(L"event").as_string()); }
		else if (name == "action")
		{
			actions.push_back(Frame::Object::Action(
				part.attribute(L"event").as_string(),
				part.attribute(L"condition").as_string(),
				part.text().get()
			));
		}
		else
		{
			setVar(part.name(),
				sf::String(part.attribute(L"type").as_string()) +
				" " + part.text().as_string()
			);
		}
	}
}

UI::Frame::Object::Progress::Progress()
{
	active = false;
	value = min = max = 0;
	bg = fg = sf::Sprite();
	fa_fg = fa_bg = FrameAnimator();
	offset = {0, 0};
	target = "";
}

UI::Frame::Object::Progress::Progress(pugi::xml_node node)
{
	parse(node);
}

void UI::Frame::Object::Progress::parse(pugi::xml_node node)
{
	setVar("name", node.attribute(L"name").as_string());
	for (auto part : node.children())
	{
		auto name = sf::String(part.name());
		if (name == "toggle") { setVar("toggle", part.attribute(L"event").as_string()); }
		else if (name == "texture")
		{
			fa_fg.loadFromFile(sf::String(part.attribute(L"path").as_string()).toAnsiString());
			fa_bg.loadFromFile(sf::String(part.attribute(L"path").as_string()).toAnsiString());
			setVar("bg", part.attribute(L"bg").as_string());
			setVar("fg", part.attribute(L"fg").as_string());
			fa_fg.setCurrentAnimation(getVar("fg").str);
			fa_bg.setCurrentAnimation(getVar("bg").str);
			auto os = tr::splitStr(part.attribute(L"offset").as_string(), " ");
			offset = {
				std::stof(os[0].toAnsiString()),
				std::stof(os[1].toAnsiString())
			};
		}
		else if (name == "value")
		{
			min = part.attribute(L"min").as_float();
			max = part.attribute(L"max").as_float();
			target = part.attribute(L"target").as_string();
			value = 0;
		}
		else
		{
			setVar(part.name(),
				sf::String(part.attribute(L"type").as_string()) +
				" " + part.text().as_string()
			);
		}
	}
}

void UI::Frame::Object::Progress::update()
{
	active = updateToggle(active, getVar("toggle"), bg.getGlobalBounds());
	if (fa_bg.getCurrentAnim())
	{
		fa_bg.setCurrentAnimation(getVar("bg").str);
		fa_bg.update();
		fa_bg.send(bg, false, false);
	}
	if (fa_fg.getCurrentAnim())
	{
		fa_fg.setCurrentAnimation(getVar("fg").str);
		fa_fg.update();
		fa_fg.send(fg, false, false);
	}
	bg.setPosition(getObjectPosition(getVar("position"), active, bg.getPosition()));
	bg.setOrigin(getObjectOrigin((sf::Vector2f)bg.getTextureRect().getSize(), getVar("origin-bg"), active, bg.getOrigin()));
	bg.setScale(getObjectScale(
		(sf::Vector2f)bg.getTextureRect().getSize(),
		getVar("scale"),
		active,
		bg.getScale()
	));
	auto v = parseText("{" + target + "}");
	value = std::stof(std::isdigit(v.toAnsiString()[0]) ? v.toAnsiString() : "0");
	fg.setPosition(bg.getGlobalBounds().getPosition() + offset);
	fg.setOrigin(getObjectOrigin((sf::Vector2f)fg.getTextureRect().getSize(), getVar("origin-fg"), active, fg.getOrigin()));
	fg.setScale(
		bg.getScale().x * (value / (max - min)),
		bg.getScale().y
	);
}

UI::Frame::Object::Action::Action()
{
	event = condition = action = "";
}

UI::Frame::Object::Action::Action(sf::String E, sf::String C, sf::String A)
{
	event = E;
	condition = C;
	action = A;
}

void UI::Frame::Object::Action::execute()
{
	auto a = tr::splitStr(action, ";");
	for (int i = 0; i < a.size(); i++) { tr::execute(a[i]); }
}

bool UI::Frame::Object::Action::isActivated()
{
	auto events = tr::splitStr(event, ";");
	for (int i = 0; i < events.size(); i++)
	{
		auto args = tr::splitStr(events[i], "-");
		if (args[0] == "mouse")
		{
			auto key = Input::strToMouse(args[2]);
			if ((args[1] == "click" && Input::isMBJustPressed(key)) ||
				(args[1] == "press" && Input::isMBPressed(key))) return true;
		}
		else if (args[0] == "key")
		{
			auto key = Input::strToKey(args[2]);
			if ((args[1] == "click" && Input::isKeyJustPressed(key)) ||
				(args[1] == "press" && Input::isKeyPressed(key))) return true;
		}
	}
	return false;
}

bool UI::Frame::Object::Action::isAvailable(bool objActive)
{
	auto conds = tr::splitStr(condition, ";");
	for (int i = 0; i < conds.size(); i++)
	{
		auto args = tr::splitStr(conds[i], "-");
		if (args[0] == "active")
		{
			if (!(
				(args[1] == "true" && objActive) ||
				(args[1] == "false" && !objActive)
			)) return false;
		}
		else if (args[0] == "worldActive")
		{
			if (!(
				(args[1] == "true" && World::active) ||
				(args[1] == "false" && !World::active)
			)) return false;
		}
		else if (args[0] == "worldLoaded")
		{
			auto active = !World::currentFile.isEmpty();
			if (!(
				(args[1] == "true" && active) ||
				(args[1] == "false" && !active)
			)) return false;
		}
		else if (args[0] == "fileExists")
		{
			auto exists = std::filesystem::exists(args[1].toAnsiString());
			if (!(
				(exists && args[2] == "true") ||
				(!exists && args[2] == "false")
			)) return false;
		}
	}
	return true;
}

UI::Frame::Object::Object()
{
	sprites.clear();
	texts.clear();
	bars.clear();
	actions.clear();
	name = "";
	active = false;
	position = {0, 0};
}

UI::Frame::Object::Object(sf::String N, bool A)
{
	sprites.clear();
	texts.clear();
	bars.clear();
	actions.clear();
	name = N;
	active = A;
	position = {0, 0};
}

void UI::Frame::Object::update()
{
	active = updateToggle(active, toggle, {0, 0, 0, 0});
	position = (pos.isEmpty() ? sf::Vector2f(0, 0) : getObjectPosition(pos, active, position));
	handle();
	for (int i = 0; i < sprites.size(); i++)
	{
		sprites[i].update();
	}
	for (int i = 0; i < texts.size(); i++)
	{
		texts[i].update();
	}
	for (int i = 0; i < bars.size(); i++)
	{
		bars[i].update();
	}
	for (int i = 0; i < actions.size(); i++)
	{
		if (actions[i].isAvailable(active) && actions[i].isActivated())
		{
			actions[i].execute();
		}
	}
}

void UI::Frame::Object::handle()
{
	if (handler.isEmpty()) { return; }
	if (tr::strContains(handler, "inventory"))
	{
		//Item Description
		auto *desc = getText("description");
		//Grid
		auto *grid = getSprite("grid");
		auto *fa = grid->fa.getCurrentAnim();
		fa->texture->setRepeated(true);
		fa->frames[0].width = Inventory::size.x * Inventory::size.z;
		fa->frames[0].height = Inventory::size.y * Inventory::size.z;
		auto startPos = grid->spr.getGlobalBounds().getPosition();
		//Items
		auto *items = getSprite("items");
		items->spr.setPosition(startPos);
		items->spr.setTexture(Inventory::invItems, true);
		items->spr.setScale(
			grid->spr.getGlobalBounds().width / items->spr.getTextureRect().width,
			grid->spr.getGlobalBounds().height / items->spr.getTextureRect().height
		);
		// items->spr.setTextureRect({
		// 	{0, 0},
		// 	sf::Vector2i(Inventory::size.x * Inventory::size.z, Inventory::size.y * Inventory::size.z)
		// });
		//Chosen cell
		if (!active) { return; }
		auto *chosen = getSprite("chosen");
		fa = chosen->fa.getCurrentAnim();
		if (!grid->spr.getGlobalBounds().contains(Input::getMousePos()))
		{
			fa->frames[0] = {0, 0, 0, 0};
			desc->activeTxt = ""; desc->setVar("acc", 1);
			return;
		}
		else fa->frames[0] = {0, 0, Inventory::size.z, Inventory::size.z};
		sf::Vector2f mPos = (sf::Vector2f)((sf::Vector2i)(Input::getMousePos() - startPos) / int(Inventory::size.z * chosen->spr.getScale().x));
		mPos = tr::clampVec(mPos, {0, 0}, {Inventory::size.x - 1, Inventory::size.y - 1});
		auto id = mPos.y * Inventory::size.x + mPos.x;
		if (id >= Inventory::inv.size())
		{
			fa->frames[0] = {0, 0, 0, 0};
			desc->activeTxt = ""; desc->setVar("acc", 1);
			return;
		}
		auto *entry = &Inventory::inv[id];
		if (entry->count <= 0)
		{
			fa->frames[0] = {0, 0, 0, 0};
			desc->activeTxt = ""; desc->setVar("acc", 1);
			return;
		}
		if (!chosen->spr.getGlobalBounds().contains(Input::getMousePos()))
		{
			auto pos = tr::splitStr(chosen->getVar("position"), " ");
			pos.erase(pos.begin() + 1, pos.begin() + 5);
			pos.insert(pos.begin() + 1, {
				std::to_string(mPos.x * Inventory::size.z * chosen->spr.getScale().x),
				std::to_string(mPos.y * Inventory::size.z * chosen->spr.getScale().y),
				std::to_string(mPos.x * Inventory::size.z * chosen->spr.getScale().x),
				std::to_string(mPos.y * Inventory::size.z * chosen->spr.getScale().y)
			});
			chosen->setVar("position", tr::partsToLine(pos, " "));
			desc->activeTxt = entry->item.name;
			if (entry->item.type != "weapon" &&
				entry->item.type != "armor" &&
				entry->item.type != "bauble") desc->activeTxt += " [" + std::to_string(entry->count) + "]";
			desc->setVar("acc", 1);
		}
		if (Input::isMBJustPressed(sf::Mouse::Left))
		{
			auto *camOwner = World::getCameraOwner();
			auto *item = &entry->item;
			if (item->useable)
			{
				for (int i = 0; i < item->effects.size(); i++)
				{
					camOwner->addEffect(item->effects[i]);
				}
				entry->count--;
			}
			if (item->type == "weapon")
			{
				auto id = item->id;
				if (camOwner->weapon.id != "null")
				{
					*item = Inventory::getItem("weapon", camOwner->weapon.id);
				}
				*entry = Inventory::ItemEntry();
				camOwner->weapon = Inventory::getWeapon(id);
			}
			else if (item->type == "bauble")
			{
				auto id = item->id;
				if (camOwner->bauble.id != "null")
				{
					*entry = Inventory::ItemEntry(Inventory::getItem("bauble", camOwner->bauble.id), 1);
				}
				else *entry = Inventory::ItemEntry();
				camOwner->bauble = Inventory::getBauble(id);
			}
			Inventory::updateGrid();
		}
		else if (Input::isKeyJustPressed(sf::Keyboard::Q))
		{
			World::throwItem(entry->item, World::getCameraOwner());
			entry->count--;
			Inventory::updateGrid();
		}
		if (entry->count <= 0)
		{
			fa->frames[0] = {0, 0, 0, 0};
			desc->activeTxt = ""; desc->setVar("acc", 1);
			return;
		}
		desc->activeTxt = entry->item.name;
		if (entry->item.type != "weapon" &&
			entry->item.type != "armor" &&
			entry->item.type != "bauble" &&
			entry->item.countable) desc->activeTxt += " [" + std::to_string(entry->count) + "]";
	}
	else if (tr::strContains(handler, "dialogue"))
	{
		auto *txt = getText("phrase");
		auto *speaker = getText("speaker");
		auto *re1 = getText("re1");
		auto *re2 = getText("re2");
		auto *re3 = getText("re3");
		auto *bg = getSprite("anim");
		bool changed = false;
		if (!Talk::active)
		{
			re1->activeTxt = re1->idleTxt = "";
			re1->setVar("acc", 1);
			re2->activeTxt = re2->idleTxt = "";
			re2->setVar("acc", 1);
			re3->activeTxt = re3->idleTxt = "";
			re3->setVar("acc", 1);
			txt->active = speaker->active = re1->active = re2->active = re3->active = false;
			auto bgclr = tr::lerpClr(
				{
					(float)bg->spr.getColor().r, (float)bg->spr.getColor().g,
					(float)bg->spr.getColor().b, (float)bg->spr.getColor().a
				},
				{0, 0, 0, 0},
				Window::getDeltaTime() * 5
			);
			bg->spr.setColor({bgclr.x, bgclr.y, bgclr.z, bgclr.w});
		}
		else
		{
			//Phrase
			Talk::Dialogue::Phrase *phrase = nullptr;
			if (auto d = Talk::getCurrentDialogue()) phrase = d->getCurrentPhrase();
			if (!phrase) return;
			txt->activeTxt = txt->idleTxt = phrase->text;
			//Speaker name
			speaker->activeTxt = speaker->idleTxt = phrase->speaker;
			auto clr = Talk::getNameColor(phrase->speaker);
			speaker->activeClr = {clr.r, clr.g, clr.b, clr.a};
			//Replies
			if (phrase->replies.size() < 3) { re3->activeTxt = re3->idleTxt = ""; re3->setVar("acc", 1); re3->active = false; }
			if (phrase->replies.size() < 2) { re2->activeTxt = re2->idleTxt = ""; re2->setVar("acc", 1); re2->active = false; }
			if (phrase->replies.size() < 1) { re1->activeTxt = re1->idleTxt = ""; re1->setVar("acc", 1); re1->active = false; return; }
			else
			{
				int cur = 0;
				for (int i = 0; i < phrase->replies.size(); i++)
				{
					if (!Talk::conditionCheck(phrase->replies[i].condition)) { continue; }
					Text *re = nullptr;
					if (cur == 0) re = re1;
					else if (cur == 1) re = re2;
					else if (cur == 2) re = re3;
					else break;
					cur++;
					re->activeTxt = re->idleTxt = active ? phrase->replies[i].text : "";
					re->active = re->txt.getGlobalBounds().contains(Input::getMousePos());
					if (re->active && Input::isMBJustPressed(sf::Mouse::Left))
					{
						for (int j = 0; j < phrase->replies[i].actions.size(); j++)
						{
							tr::execute(phrase->replies[i].actions[j]);
						}
						txt->setVar("acc", 1);
						txt->txt.setString("");
						changed = true;
					}
				}
			}
		}
		//Background
		if (!CSManager::active)
		{
			World::getCurrentLevel()->cam.doUpdate = true;
			CSManager::music.setVolume(tr::clamp(
				CSManager::music.getVolume() - Window::getDeltaTime() * 10,
				0, Window::getVar("musicVolume")
			));
			return;
		}
		if (!CSManager::current.x)
		{
			CSManager::FrameCutscene::Change *change = nullptr;
			CSManager::FrameCutscene *cs = nullptr;
			if (cs = &CSManager::frames[CSManager::current.y])
			 	change = cs->getChange(Talk::getCurrentDialogue()->getCurrentPhrase()->name);
			auto c = bg->getVar("clr").num;
			if (CSManager::currentMusic != change->musicPath)
			{
				CSManager::music.setVolume(tr::clamp(
					CSManager::music.getVolume() - Window::getDeltaTime() * 10,
					0, Window::getVar("musicVolume")
				));
			}
			else
			{
				CSManager::music.setVolume(tr::clamp(
					CSManager::music.getVolume() + Window::getDeltaTime() * 10,
					0, Window::getVar("musicVolume")
				));
			}
			if (CSManager::music.getVolume() == 0)
			{
				CSManager::currentMusic = change->musicPath;
				if (!CSManager::currentMusic.isEmpty()) CSManager::music.openFromFile(CSManager::currentMusic);
				CSManager::music.play();
			}
			if (!CSManager::currentMusic.isEmpty())
			{
				CSManager::music.setVolume(tr::clamp(
					CSManager::music.getVolume() + Window::getDeltaTime() * 10,
					0, Window::getVar("musicVolume")
				));
			}
			if (change->anim != cs->frame.getCurrentAnim()->name)
			{
				if ((int)c > 0)
				{
					c = tr::lerp(c, 0, Window::getDeltaTime() * 10);
					txt->setVar("acc", 0);
					if ((int)c <= 0)
					{
						cs->frame.setCurrentAnimation(change->anim);
					}
				}
				else
				{
					cs->frame.setCurrentAnimation(change->anim);
					c = tr::lerp(c, 255, Window::getDeltaTime() * 10);
				}
			}
			else
			{
				cs->frame.setCurrentAnimation(change->anim);
				c = tr::lerp(c, 255, Window::getDeltaTime() * 10);
			}
			if (change->skippable && Input::isMBJustPressed(sf::Mouse::Left) && !changed)
			{
				txt->setVar("acc", txt->activeTxt.getSize());
			}
			cs->frame.update();
			cs->frame.send(bg->spr, false, false);
			bg->spr.scale(
				Window::getSize().x / bg->spr.getGlobalBounds().width,
				Window::getSize().y / bg->spr.getGlobalBounds().height
			);
			bg->spr.setColor({c, c, c, c});
			bg->setVar("clr", c);
		}
		else
		{
			bg->spr.setColor({0, 0, 0, 0});
			auto c = CSManager::worlds[CSManager::current.y].getCurrentChange();
			if (!CSManager::shouldEnd) Talk::active = (c->startPhraseOnEnd ? c->current >= c->duration : true);
			CSManager::music.setVolume(tr::clamp(
				CSManager::music.getVolume() - Window::getDeltaTime() * 10,
				0, Window::getVar("musicVolume")
			));
		}
	}
	else if (tr::strContains(handler, "acsMenu"))
	{
		auto *camOwner = World::getCameraOwner(); if (!camOwner) { return; }
		auto *desc = getText("description"); if (!desc) { return; }
		bool setDesc = false;
		//Weapon
		auto *wpn = getSprite("weapon");
		wpn->fa.getCurrentAnim()->currentFrame = camOwner->weapon.id != "null";
		if (wpn->spr.getGlobalBounds().contains(Input::getMousePos()))
		{
			setDesc = true;
			auto item = Inventory::getItem("weapon", camOwner->weapon.id);
			if (desc->activeTxt != item.name && item.name != "null") { desc->activeTxt = item.name; desc->setVar("acc", 1); }
			if (Input::isMBJustPressed(sf::Mouse::Left))
			{
				Inventory::addItem(item.type, item.id);
				camOwner->weapon = Weapon();
				desc->activeTxt = ""; desc->setVar("acc", 1);
			}
		}
		//Armor
		auto *arm = getSprite("armor");
		arm->fa.getCurrentAnim()->currentFrame = 0; //Not implemented yet
		//Bauble
		auto *bbl = getSprite("bauble");
		bbl->fa.getCurrentAnim()->currentFrame = camOwner->bauble.id != "null";
		if (bbl->spr.getGlobalBounds().contains(Input::getMousePos()))
		{
			setDesc = true;
			auto item = Inventory::getItem("bauble", camOwner->bauble.id);
			if (desc->activeTxt != item.name && item.name != "null") { desc->activeTxt = item.name; desc->setVar("acc", 1); }
			if (Input::isMBJustPressed(sf::Mouse::Left))
			{
				Inventory::addItem(item.type, item.id);
				camOwner->bauble = Bauble();
				desc->activeTxt = ""; desc->setVar("acc", 1);
			}
		}
		if (!setDesc) { desc->activeTxt = ""; desc->setVar("acc", 1); }
	}
	else if (tr::strContains(handler, "console"))
	{
		Input::active = false;
		auto txt = getText("console");
		if (Window::hasEvent(sf::Event::TextEntered))
		{
			auto text = txt->activeTxt;
			auto code = Window::getEvent(sf::Event::TextEntered).text.unicode;
			if (code == 8) { if (text.getSize() > 1) text.erase(text.getSize() - 1); }
			else if (code == 9) { text += "\t"; }
			else if (code == 13)
			{
				auto cmd = tr::splitStr(text.substring(1), ";");
				for (int i = 0; i < cmd.size(); i++)
				{
					tr::execute(cmd[i]);
				}
				text = ">";
			}
			else if (code == 99 && Input::isKeyPressed(sf::Keyboard::LControl))
			{
				sf::Clipboard::setString(text.substring(1));
			}
			else if (code == 118 && Input::isKeyPressed(sf::Keyboard::LControl))
			{
				text += sf::Clipboard::getString();
			}
			else if (code != 27) { text += sf::String(code); }
			txt->activeTxt = text;
		}
	}
	else if (tr::strContains(handler, "hint"))
	{
		auto hint = Window::getVar("hint").str;
		auto timer = Window::getVar("hintTimer").num;
		if (timer <= 0) { Window::setVar("hint", ""); }
		Window::setVar("hintTimer", timer - Window::getDeltaTime());

		auto txt = getText("hint");
		txt->activeTxt = hint;
		txt->idleTxt = hint;
	}
	else if (tr::strContains(handler, "scroller"))
	{
		auto bar = getProgress("scroller");
		if (bar->bg.getGlobalBounds().contains(Input::getMousePos()) &&
			Input::isMBJustPressed(sf::Mouse::Left)) { bar->active = true; }
		if (Input::isMBJustReleased(sf::Mouse::Left)) { bar->active = false; }
		if (!bar->active) return;

		auto mx = Input::getMousePos().x;
		auto bx = bar->bg.getGlobalBounds().left;
		auto range = bar->max - bar->min;

		bar->value = tr::clamp((mx - bx) / bar->bg.getGlobalBounds().width * range, bar->min, bar->max);

		auto path = tr::splitStr(bar->target, "-");
		Script::getProgrammable(bar->target)->setVar(path[2], bar->value);
	}
}

void UI::Frame::Object::draw()
{
	for (int i = 0; i < sprites.size(); i++) { Window::draw(sprites[i].spr); }
	for (int i = 0; i < texts.size(); i++) { Window::draw(texts[i].txt); }
	for (int i = 0; i < bars.size(); i++)
	{
		Window::draw(bars[i].bg);
		Window::draw(bars[i].fg);
	}
}

UI::Frame::Object::Sprite *UI::Frame::Object::getSprite(sf::String name)
{
	for (int i = 0; i < sprites.size(); i++)
	{
		if (sprites[i].getVar("name") == name) { return &sprites[i]; }
	}
	return nullptr;
}

UI::Frame::Object::Text *UI::Frame::Object::getText(sf::String name)
{
	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i].getVar("name") == name) { return &texts[i]; }
	}
	return nullptr;
}

UI::Frame::Object::Progress *UI::Frame::Object::getProgress(sf::String name)
{
	for (int i = 0; i < bars.size(); i++)
	{
		if (bars[i].getVar("name") == name) { return &bars[i]; }
	}
	return nullptr;
}

UI::Frame::Frame()
{
	objects.clear();
	name = "";
}

UI::Frame::Frame(sf::String N)
{
	objects.clear();
	name = N;
}

void UI::Frame::update()
{
	for (int i = 0; i < objects.size(); i++)
	{
		objects[i].update();
	}
}

void UI::Frame::draw()
{
	for (int i = 0; i < objects.size(); i++)
	{
		objects[i].draw();
	}
}

UI::Frame::Object *UI::Frame::getObject(sf::String name)
{
	for (int i = 0; i < objects.size(); i++)
	{
		if (objects[i].name == name) { return &objects[i]; }
	}
	return nullptr;
}

UI::Frame::Object *UI::Frame::getObjectByHandler(sf::String handler)
{
	for (int i = 0; i < objects.size(); i++)
	{
		if (objects[i].handler == handler) { return &objects[i]; }
	}
	return nullptr;
}

void UI::init()
{
	frames.clear();
	currentFrame = "";
}

void UI::loadFromFile(std::string filename, bool reload)
{
	if (reload) { reloadUI = true; currentFile = filename; return; }
	init();
	currentFile = filename;
	pugi::xml_document document;
	document.load_string(AssetManager::getText(filename).toWideString().c_str());
	for (auto frame : document.children())
	{
		if (sf::String(frame.name()) != "frame") { continue; }
		auto f = Frame(frame.attribute(L"name").as_string());
		for (auto object : frame.children())
		{
			if (sf::String(object.name()) != "object") { continue; }
			auto obj = Frame::Object(object.attribute(L"name").as_string());
			for (auto part : object.children())
			{
				auto name = sf::String(part.name());
				if (name == "sprite") { obj.sprites.push_back(Frame::Object::Sprite(part)); }
				else if (name == "text") { obj.texts.push_back(Frame::Object::Text(part)); }
				else if (name == "progress") { obj.bars.push_back(Frame::Object::Progress(part)); }
				else if (name == "toggle")
				{
					obj.toggle = sf::String(part.attribute(L"event").as_string());
				}
				else if (name == "action")
				{
					obj.actions.push_back(Frame::Object::Action(
						part.attribute(L"event").as_string(),
						part.attribute(L"condition").as_string(),
						part.text().get()
					));
				}
				else if (name == "position")
				{
					obj.pos = sf::String(part.attribute(L"type").as_string()) +
						" " + part.text().as_string();
				}
				else if (name == "update")
				{
					obj.handler = part.attribute(L"type").as_string();
				}
			}
			f.objects.push_back(obj);
		}
		frames.push_back(f);
	}
	currentFrame = frames[0].name;
}

UI::Frame *UI::getCurrentFrame()
{
	for (int i = 0; i < frames.size(); i++)
	{
		if (frames[i].name == currentFrame) { return &frames[i]; }
	}
	return nullptr;
}

void UI::update()
{
	if (reloadUI) { loadFromFile(currentFile); reloadUI = false; }
	getCurrentFrame()->update();
}

void UI::draw()
{
	getCurrentFrame()->draw();
}

sf::Vector2f UI::getObjectPosition(sf::String var, bool active, sf::Vector2f old)
{
	auto pos = tr::splitStr(var, " ");
	auto type = tr::splitStr(pos[0], "-");
	auto xy = sf::Vector2f(
		std::stof(pos[active ? 3 : 1].toAnsiString()),
		std::stof(pos[active ? 4 : 2].toAnsiString())
	);
	if (type[0] == "relative")
	{
		sf::Vector2f result = {
			Window::getSize().x * xy.x,
			Window::getSize().y * xy.y
		};
		if (type[1] == "set") { return result; }
		else if (type[1] == "lerp") { return tr::lerpVec(old, result, 5 * Window::getDeltaTime()); }
	}
	else if (type[0] == "absolute")
	{
		if (type[1] == "set") { return xy; }
		else if (type[1] == "lerp") { return tr::lerpVec(old, xy, 5 * Window::getDeltaTime()); }
	}
	else if (type[0] == "inherit")
	{
		sf::FloatRect rect = {{0, 0}, Window::getSize()};
		auto args = tr::splitStr(pos[5], "-");
		if (type[1] == "obj") rect = {getFrame(args[0])->getObject(args[1])->position, rect.getSize()};
		else if (type[1] == "spr") rect = getFrame(args[0])->getObject(args[1])->getSprite(args[2])->spr.getGlobalBounds();
		else if (type[1] == "txt") rect = getFrame(args[0])->getObject(args[1])->getText(args[2])->txt.getGlobalBounds();

		if (type[2] == "rel")
		{
			return rect.getPosition() + sf::Vector2f(
				rect.width * xy.x,
				rect.height * xy.y
			);
		}
		else if (type[2] == "abs")
		{
			return rect.getPosition() + xy;
		}
	}
	return old;
}

sf::Vector2f UI::getObjectOrigin(sf::Vector2f size, sf::String var, bool active, sf::Vector2f old)
{
	auto origin = tr::splitStr(var, " ");
	auto xy = sf::Vector2f(
		std::stof(origin[active ? 3 : 1].toAnsiString()),
		std::stof(origin[active ? 4 : 2].toAnsiString())
	);
	if (origin[0] == "relative") return {
		size.x * xy.x,
		size.y * xy.y
	};
	else if (origin[0] == "relative-lerp") return tr::lerpVec(
		old,
		{
			size.x * xy.x,
			size.y * xy.y
		},
		5 * Window::getDeltaTime()
	);
	else if (origin[0] == "absolute") return xy;
	else if (origin[0] == "absolute-lerp") return tr::lerpVec(old, xy, 5 * Window::getDeltaTime());
	return old;
}

sf::Vector2f UI::getObjectScale(sf::Vector2f size, sf::String var, bool active, sf::Vector2f curScale)
{
	auto scale = tr::splitStr(var, " ");
	sf::Vector2f xy;
	if (scale.size() > 1) xy = sf::Vector2f(
		std::stof(scale[active ? 3 : 1].toAnsiString()),
		std::stof(scale[active ? 4 : 2].toAnsiString())
	);
	if (scale[0] == "stretch") return {
		Window::getSize().x / size.x,
		Window::getSize().y / size.y
	};
	else if (scale[0] == "set") return xy;
	else if (scale[0] == "lerp") return tr::lerpVec(
		curScale,
		xy,
		5 * Window::getDeltaTime()
	);
	return curScale;
}

bool UI::containsMouse()
{
	auto *f = getCurrentFrame();
	for (int i = 0; i < f->objects.size(); i++)
	{
		auto *o = &f->objects[i];
		for (int j = 0; j < o->sprites.size(); j++)
		{
			if (o->sprites[j].spr.getGlobalBounds().contains(Input::getMousePos())) { return true; }
		}
		for (int j = 0; j < o->texts.size(); j++)
		{
			if (o->texts[j].txt.getGlobalBounds().contains(Input::getMousePos())) { return true; }
		}
	}
	return false;
}

bool UI::containsMouse(UI::Frame::Object *obj)
{
	for (int j = 0; j < obj->sprites.size(); j++)
	{
		if (obj->sprites[j].spr.getGlobalBounds().contains(Input::getMousePos())) { return true; }
	}
	for (int j = 0; j < obj->texts.size(); j++)
	{
		if (obj->texts[j].txt.getGlobalBounds().contains(Input::getMousePos())) { return true; }
	}
	return false;
}

void UI::setFrame(sf::String frame)
{
	for (int i = 0; i < frames.size(); i++)
	{
		if (frames[i].name == frame) { currentFrame = frame; }
	}
}

UI::Frame *UI::getFrame(sf::String name)
{
	for (int i = 0; i < frames.size(); i++)
	{
		if (frames[i].name == name) { return &frames[i]; }
	}
	return nullptr;
}

bool UI::updateToggle(bool active, sf::String toggle, sf::FloatRect hitbox)
{
	if (toggle.isEmpty()) { return active; }
	auto args = tr::splitStr(toggle, "-");
	if (tr::strContains(args[0], "always")) { return std::stoi(args[1].toAnsiString()); }
	else if (tr::strContains(args[0], "mouseHover"))
	{
		return hitbox.contains(Input::getMousePos());
	}
	else if (tr::strContains(args[0], "key"))
	{
		if (tr::strContains(args[1], "click"))
		{
			if (Input::isKeyJustPressed(Input::strToKey(args[2]))) { return !active; }
			else { return active; }
		}
		else if (tr::strContains(args[1], "press"))
		{
			return Input::isKeyPressed(Input::strToKey(args[2]));
		}
	}
	else if (tr::strContains(args[0], "mouse"))
	{
		if (tr::strContains(args[1], "click"))
		{
			if (Input::isMBJustPressed(Input::strToMouse(args[2]))) { return !active; }
			else { return active; }
		}
		else if (tr::strContains(args[1], "press"))
		{
			return Input::isMBPressed(Input::strToMouse(args[2]));
		}
	}
	else if (tr::strContains(args[0], "txtReady"))
	{
		auto *txt = getFrame(args[1])->getObject(args[2])->getText(args[3]);
		return txt->txt.getString().getSize() == txt->activeTxt.getSize() && txt->active;
	}
	else if (tr::strContains(args[0], "txtNotEmpty"))
	{
		auto *txt = getFrame(args[1])->getObject(args[2])->getText(args[3]);
		return !txt->txt.getString().isEmpty();
	}
	else if (tr::strContains(args[0], "btn"))
	{
		return getFrame(args[1])->getObject(args[2])->getSprite(args[3])->active;
	}
	else if (tr::strContains(args[0], "txt"))
	{
		return getFrame(args[1])->getObject(args[2])->getText(args[3])->active;
	}
	else if (tr::strContains(args[0], "obj"))
	{
		return getFrame(args[1])->getObject(args[2])->active;
	}
	else if (tr::strContains(args[0], "talk"))
	{
		if (args[1] == "active") return Talk::active;
		if (args[1] == "inactive") return !Talk::active;
	}
	else if (tr::strContains(args[0], "lvl"))
	{
		return World::getCurrentLevel()->getVar(args[1]).num == 1;
	}
	else if (tr::strContains(args[0], "cmp"))
	{
		float tester = std::stof(args[args.size() - 1].toAnsiString());
		auto cmp = args[args.size() - 2];
		float value;
		if (args[1] == "input")
		{
			Input::Controller *c = Input::getControl(args[2]);
			value = c->getVariable(args[3])->value;
		}
		if (cmp == "eq") return value == tester;
		if (cmp == "ne") return value != tester;
		if (cmp == "gt") return value >  tester;
		if (cmp == "ge") return value >= tester;
		if (cmp == "lt") return value <  tester;
		if (cmp == "le") return value <= tester;
	}
	return false;
}

sf::String UI::parseText(sf::String txt)
{
	sf::String result = txt;
	while (tr::strContains(result, "{"))
	{
		auto start = result.find("{"), end = result.find("}");
		auto path = result.substring(start + 1, end - start - 1);
		auto var = tr::splitStr(path, "-");
		sf::String value;
		Programmable *prog = Script::getProgrammable(path);
		if (var[0] == "input" && !prog) { prog = Window::getProgrammable(); value = Input::getControl(var[1])->getKeyByVar(var[2])->toString(); }
		if (prog == nullptr) { result.replace(start, end - start + 1, "?"); continue; }
		if (var[1] == "str") value = prog->getVar(var[2]);
		else if (var[1] == "int") value = std::to_string((int)prog->getVar(var[2]).num);
		else if (var[1] == "num") { value = std::to_string(prog->getVar(var[2]).num); value = value.substring(0, value.find(".") + 3); }
		result.replace(start, end - start + 1, value);
	}
	return result;
}