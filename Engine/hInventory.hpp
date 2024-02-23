#ifndef trInventory
#define trInventory

#include "hAnimation.hpp"
#include "hProgrammable.hpp"

class Weapon;
class Bauble;

class Inventory
{
public:
	struct Effect
	{
		sf::Clock timer;
		float frequency;
		int used, usageCount;
		Programmable::Variable var;
		bool setOrAdd;
		Effect();
		Effect(Programmable::Variable effect, float freq, int usageCnt, bool SetOrAdd);
	};
	struct Item
	{
		FrameAnimator fa;
		sf::Sprite spr;
		sf::String name, type, id;
		std::vector<Effect> effects;
		bool countable, useable;
		Item();
		void updateSpr();
	};
	struct ItemEntry
	{
		Item item;
		unsigned short count;
		ItemEntry();
		ItemEntry(Item itm, sf::Uint16 cnt);
	};
	static std::vector<ItemEntry> inv;
	static void init();
	static Item getItem(sf::String type, sf::String id);
	static Item getItem(sf::String id);
	static Weapon getWeapon(sf::String id);
	static Bauble getBauble(sf::String id);
	static bool addItem(sf::String type, sf::String id, unsigned short count = 1);
	static bool addItem(sf::String id, unsigned short count = 1);
	static sf::Glsl::Vec4 size;
	static sf::Texture invItems;
	static void updateGrid();
private:
	static sf::RenderTexture itemsCanvas;
	static std::vector<Item> items;
	static std::vector<Weapon> weapons;
	static std::vector<Bauble> baubles;
};

#endif