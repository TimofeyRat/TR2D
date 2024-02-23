#include "hInventory.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hWearable.hpp"

#include <iostream>

std::vector<Inventory::Item> Inventory::items;
std::vector<Weapon> Inventory::weapons;
std::vector<Bauble> Inventory::baubles;
std::vector<Inventory::ItemEntry> Inventory::inv;
sf::Glsl::Vec4 Inventory::size;
sf::Texture Inventory::invItems;
sf::RenderTexture Inventory::itemsCanvas;

Inventory::Effect::Effect()
{
	timer.restart();
	frequency = used = usageCount = 0;
	var = Programmable::Variable();
	setOrAdd = false;
}

Inventory::Effect::Effect(Programmable::Variable effect, float freq, int usageCnt, bool SetOrAdd)
{
	timer.restart();
	frequency = freq;
	used = 0;
	usageCount = usageCnt;
	var = effect;
	setOrAdd = SetOrAdd;
}

Inventory::Item::Item()
{
	fa = FrameAnimator();
	spr = sf::Sprite();
	name = type = id = "null";
	effects.clear();
	countable = useable = true;
}

void Inventory::Item::updateSpr()
{
	fa.setCurrentAnimation(id);
	fa.update();
	fa.send(spr);
}

Inventory::ItemEntry::ItemEntry()
{
	item = Item();
	count = 0;
}

Inventory::ItemEntry::ItemEntry(Inventory::Item itm, sf::Uint16 cnt)
{
	item = itm;
	count = cnt;
}

void Inventory::init()
{
	size = {
		Window::getVar("invSizeX"),
		Window::getVar("invSizeY"),
		Window::getVar("invCellSize"),
		Window::getVar("invCellInner")
	};
	itemsCanvas.create(size.x * size.z, size.y * size.z);
	items.clear();
	weapons.clear();
	baubles.clear();
	inv.clear();
	Item item;
	Weapon wpn;
	Bauble bbl;
	for (auto file : AssetManager::getTexts("res/items"))
	{
		for (auto line : tr::splitStr(AssetManager::getText(file), "\n"))
		{
			auto args = tr::splitStr(line, " ");
			if (tr::strContains(args[0], "#")) { continue; }
			//Item
			else if (tr::strContains(args[0], "EndItem"))
			{
				items.push_back(item);
			}
			else if (tr::strContains(args[0], "Item"))
			{
				item = Item();
				auto id = tr::splitStr(args[1], ":");
				item.type = id[0];
				item.id = id[1];
			}
			else if (tr::strContains(args[0], "ItmName"))
			{
				item.name = args[1];
			}
			else if (tr::strContains(args[0], "ItmAnimation"))
			{
				item.fa.loadFromFile(args[1]);
			}
			else if (tr::strContains(args[0], "ItmEffect"))
			{
				Programmable::Variable var;
				var.name = args[1];
				if (tr::strContains(args[2], "str")) { var.str = args[3]; }
				else if (tr::strContains(args[2], "num")) { var.num = std::stof(args[3].toAnsiString()); }
				item.effects.push_back(Effect(
					var,
					std::stof(args[4].toAnsiString()),
					std::stoi(args[5].toAnsiString()),
					std::stoi(args[6].toAnsiString())
				));
			}
			else if (tr::strContains(args[0], "ItmCountable"))
			{
				item.countable = std::stoi(args[1].toAnsiString());
			}
			else if (tr::strContains(args[0], "ItmUseable"))
			{
				item.useable = std::stoi(args[1].toAnsiString());
			}
			//Weapon
			else if (tr::strContains(args[0], "EndWeapon"))
			{
				wpn.timer.restart();
				weapons.push_back(wpn);
			}
			else if (tr::strContains(args[0], "Weapon"))
			{
				wpn = Weapon();
				wpn.id = args[1];
			}
			else if (tr::strContains(args[0], "WpnAnimation"))
			{
				wpn.fa.loadFromFile(args[1]);
				wpn.fa.setCurrentAnimation(wpn.id);
			}
			else if (tr::strContains(args[0], "WpnEffect"))
			{
				Programmable::Variable var;
				var.name = args[1];
				if (tr::strContains(args[2], "str")) { var.str = args[3]; }
				else if (tr::strContains(args[2], "num")) { var.num = std::stof(args[3].toAnsiString()); }
				wpn.effects.push_back(Effect(
					var,
					std::stof(args[4].toAnsiString()),
					std::stoi(args[5].toAnsiString()),
					std::stoi(args[6].toAnsiString())
				));
			}
			else if (tr::strContains(args[0], "WpnDelay"))
			{
				wpn.useDelay = std::stof(args[1].toAnsiString());
			}
			else if (tr::strContains(args[0], "WpnOrigin"))
			{
				wpn.origin = {
					std::stof(args[1].toAnsiString()),
					std::stof(args[2].toAnsiString())
				};
			}
			else if (tr::strContains(args[0], "WpnRotation"))
			{
				wpn.rotation = std::stof(args[1].toAnsiString());
			}
			else if (tr::strContains(args[0], "WpnScale"))
			{
				wpn.scale = std::stof(args[1].toAnsiString());
			}
			else if (tr::strContains(args[0], "WpnType"))
			{
				if (tr::strContains(args[1], "Melee")) wpn.meleeOrRange = false;
				else if (tr::strContains(args[1], "Range")) { wpn.meleeOrRange = true; }
			}
			else if (tr::strContains(args[0], "EndBauble"))
			{
				bbl.timer.restart();
				baubles.push_back(bbl);
			}
			else if (tr::strContains(args[0], "Bauble"))
			{
				bbl = Bauble();
				bbl.id = args[1];
			}
			else if (tr::strContains(args[0], "BblAnimation"))
			{
				bbl.fa.loadFromFile(args[1]);
				bbl.fa.setCurrentAnimation(bbl.id);
			}
			else if (tr::strContains(args[0], "BblEffect"))
			{
				Programmable::Variable var;
				var.name = args[1];
				if (tr::strContains(args[2], "str")) { var.str = args[3]; }
				else if (tr::strContains(args[2], "num")) { var.num = std::stof(args[3].toAnsiString()); }
				bbl.effects.push_back(Effect(
					var,
					std::stof(args[4].toAnsiString()),
					std::stoi(args[5].toAnsiString()),
					std::stoi(args[6].toAnsiString())
				));
			}
			else if (tr::strContains(args[0], "BblDelay"))
			{
				bbl.useDelay = std::stof(args[1].toAnsiString());
			}
			else if (tr::strContains(args[0], "BblOrigin"))
			{
				bbl.origin = {
					std::stof(args[1].toAnsiString()),
					std::stof(args[2].toAnsiString())
				};
			}
			else if (tr::strContains(args[0], "BblScale"))
			{
				bbl.scale = std::stof(args[1].toAnsiString());
			}
			else if (tr::strContains(args[0], "BblRotation"))
			{
				bbl.rotation = std::stof(args[1].toAnsiString());
			}
			else if (tr::strContains(args[0], "BblType"))
			{
				if (tr::strContains(args[1], "Idle")) { bbl.idleOrActive = false; }
				else if (tr::strContains(args[1], "Active")) { bbl.idleOrActive = true; }
			}
			else if (tr::strContains(args[0], "BblPlacement"))
			{
				bbl.bone = args[1];
			}
		}
	}
}

Inventory::Item Inventory::getItem(sf::String type, sf::String id)
{
	for (int i = 0; i < items.size(); i++)
	{
		if (items[i].type == type && items[i].id == id) { return items[i]; }
	}
	return Item();
}

Inventory::Item Inventory::getItem(sf::String name)
{
	auto data = tr::splitStr(name, ":");
	return getItem(data[0], data[1]);
}

bool Inventory::addItem(sf::String type, sf::String id, unsigned short count)
{
	for (int i = 0; i < inv.size(); i++)
	{
		if (inv[i].item.type == type && inv[i].item.id == id && inv[i].item.countable) { inv[i].count += count; updateGrid(); return true; }
		if (inv[i].item.type == "null" && inv[i].item.id == "null") { inv[i] = ItemEntry(getItem(type, id), 1); updateGrid(); return true; }
	}
	if (inv.size() == size.x * size.y) { return false; }
	ItemEntry ie;
	ie.count = count;
	ie.item = getItem(type, id);
	inv.push_back(ie);

	updateGrid();
	
	return true;
}

bool Inventory::addItem(sf::String id, unsigned short count)
{
	auto data = tr::splitStr(id, ":");
	return addItem(data[0], data[1], count);
}

Weapon Inventory::getWeapon(sf::String id)
{
	for (int i = 0; i < weapons.size(); i++)
	{
		if (weapons[i].id == id) { return weapons[i]; }
	}
	return Weapon();
}

Bauble Inventory::getBauble(sf::String id)
{
	for (int i = 0; i < baubles.size(); i++)
	{
		if (baubles[i].id == id) { return baubles[i]; }
	}
	return Bauble();
}

void Inventory::updateGrid()
{
	itemsCanvas.clear({0, 0, 0, 0});
	for (int y = 0; y < size.y; y++)
	{
		for (int x = 0; x < size.x; x++)
		{
			auto id = y * size.x + x;
			if (id >= inv.size()) { continue; }
			auto *entry = &inv[id];
			if (entry == nullptr) { continue; }
			if (entry->count <= 0) { *entry = ItemEntry(); continue; }
			entry->item.updateSpr();
			entry->item.spr.setPosition(x * size.z + size.z / 2, y * size.z + size.z / 2);
			entry->item.spr.setOrigin((sf::Vector2f)entry->item.spr.getTextureRect().getSize() / 2.0f);
			auto s = sf::Vector2f(
				entry->item.fa.getCurrentAnim()->frames[0].width,
				entry->item.fa.getCurrentAnim()->frames[0].height
			);
			entry->item.spr.setScale(size.w / s.x, size.w / s.y);
			itemsCanvas.draw(entry->item.spr);
		}
	}
	itemsCanvas.display();
	invItems = itemsCanvas.getTexture();
}