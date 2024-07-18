#include "hInventory.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include "hWearable.hpp"

#include <pugixml.hpp>

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
		Window::getVar("invCellMax")
	};
	itemsCanvas.create(size.x * size.w, size.y * size.w);
	items.clear();
	weapons.clear();
	baubles.clear();
	inv.clear();
	for (auto path : AssetManager::getTexts("items"))
	{
		pugi::xml_document file;
		file.load_string(AssetManager::getText(path).toWideString().c_str());
		for (auto node : file.children())
		{
			auto type = sf::String(node.name());
			if (type == "item")
			{
				Item item;
				auto id = tr::splitStr(node.attribute(L"id").as_string(), ":");
				item.type = id[0];
				item.id = id[1];
				item.name = node.attribute(L"name").as_string();
				item.fa.loadFromFile(pugi::as_utf8(node.attribute(L"anim").as_string()));
				item.countable = node.attribute(L"countable").as_bool();
				item.useable = node.attribute(L"useable").as_bool();
				for (auto effect : node.children())
				{
					if (sf::String(effect.name()) == "effect")
					{
						Programmable::Variable var;
						var.name = effect.attribute(L"name").as_string();
						var.num = effect.attribute(L"num").as_float();
						var.str = effect.attribute(L"str").as_string();
						bool type = false;
						if (sf::String(effect.attribute(L"type").as_string()) == "set") { type = false; }
						if (sf::String(effect.attribute(L"type").as_string()) == "add") { type = true; }
						item.effects.push_back(Effect(
							var,
							effect.attribute(L"frequency").as_float(),
							effect.attribute(L"usages").as_float(),
							type
						));
					}
				}
				items.push_back(item);
			}
			if (type == "weapon")
			{
				Weapon wpn;
				wpn.id = node.attribute(L"id").as_string();
				sf::String anim = node.attribute(L"anim").as_string();
				if (!anim.isEmpty()) wpn.fa.loadFromFile(anim);
				wpn.fa.setCurrentAnimation(wpn.id);
				wpn.useDelay = node.attribute(L"delay").as_float();
				auto origin = tr::splitStr(node.attribute(L"origin").as_string(L"0 0"), " ");
				wpn.origin = {
					std::stof(origin[0].toAnsiString()),
					std::stof(origin[1].toAnsiString())
				};
				wpn.rotation = node.attribute(L"rotation").as_float();
				wpn.scale = node.attribute(L"scale").as_float();
				wpn.type = node.attribute(L"type").as_string();
				wpn.showOnIdle = node.attribute(L"showOnIdle").as_bool();
				for (auto effect : node.children())
				{
					if (sf::String(effect.name()) == "effect")
					{
						Programmable::Variable var;
						var.name = effect.attribute(L"name").as_string();
						var.num = effect.attribute(L"num").as_float();
						var.str = effect.attribute(L"str").as_string();
						bool type = false;
						if (sf::String(effect.attribute(L"type").as_string()) == "set") { type = false; }
						if (sf::String(effect.attribute(L"type").as_string()) == "add") { type = true; }
						wpn.effects.push_back(Effect(
							var,
							effect.attribute(L"frequency").as_float(),
							effect.attribute(L"usages").as_float(),
							type
						));
					}
				}
				weapons.push_back(wpn);
			}
			if (type == "bauble")
			{
				Bauble bbl;
				bbl.id = node.attribute(L"id").as_string();
				bbl.fa.loadFromFile(pugi::as_utf8(node.attribute(L"anim").as_string()));
				bbl.fa.setCurrentAnimation(bbl.id);
				bbl.useDelay = node.attribute(L"delay").as_float();
				auto origin = tr::splitStr(node.attribute(L"origin").as_string(), " ");
				bbl.origin = {
					std::stof(origin[0].toAnsiString()),
					std::stof(origin[1].toAnsiString())
				};
				bbl.scale = node.attribute(L"scale").as_float();
				bbl.rotation = node.attribute(L"rotation").as_float();
				bbl.idleOrActive = (sf::String(node.attribute(L"type").as_string()) == "active");
				bbl.bone = node.attribute(L"placement").as_string();
				for (auto effect : node.children())
				{
					if (sf::String(effect.name()) == "effect")
					{
						Programmable::Variable var;
						var.name = effect.attribute(L"name").as_string();
						var.num = effect.attribute(L"num").as_float();
						var.str = effect.attribute(L"str").as_string();
						bool type = false;
						if (sf::String(effect.attribute(L"type").as_string()) == "set") { type = false; }
						if (sf::String(effect.attribute(L"type").as_string()) == "add") { type = true; }
						bbl.effects.push_back(Effect(
							var,
							effect.attribute(L"frequency").as_float(),
							effect.attribute(L"usages").as_float(),
							type
						));
					}
				}
				baubles.push_back(bbl);
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
			entry->item.spr.setPosition(x * size.w + size.w / 2, y * size.w + size.w / 2);
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

void Inventory::save(pugi::xml_node node)
{
	for (int i = 0; i < inv.size(); i++)
	{
		auto e = node.append_child(L"entry");
		e.append_attribute(L"count") = inv[i].count;
		e.append_attribute(L"item") = sf::String(
			inv[i].item.type + ":" + inv[i].item.id
		).toWideString().c_str();
	}
}

void Inventory::load(pugi::xml_node node)
{
	inv.clear();
	for (auto e : node.children())
	{
		addItem(e.attribute(L"item").as_string(), e.attribute(L"count").as_uint());
	}
}

bool Inventory::hasItem(sf::String type, sf::String id, unsigned short count)
{
	for (int i = 0; i < inv.size(); i++)
	{
		if (inv[i].item.type == type && inv[i].item.id == id) { return (count == 0 ? true : inv[i].count >= count); }
	}
	return false;
}

bool Inventory::hasItem(sf::String id, unsigned short count)
{
	auto data = tr::splitStr(id, ":");
	return hasItem(data[0], data[1], count);
}