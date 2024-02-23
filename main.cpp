#include "Engine/hAssets.hpp"
#include "Engine/hWindow.hpp"
#include "Engine/hUI.hpp"
#include "Engine/hWorld.hpp"
#include "Engine/hInput.hpp"

#include "Engine/hInventory.hpp"
#include "Engine/hDialogue.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
	Window::init(argc, argv);
	AssetManager::init();

	Input::init();
	UI::loadFromFile("res/ui/mainMenu.trui");

	Window::resetTime();

	while (Window::isOpen())
	{
		if (World::getActive() && !Inventory::inv.size())
		{
			Inventory::addItem("food:shaurma", UINT16_MAX);
			Inventory::addItem("food:meat", UINT16_MAX);
			Inventory::addItem("weapon:sticky_blade");
			Inventory::addItem("bauble:speed");
		}
		Window::update();

		Input::update();
		World::update();

		Window::clear();
		World::draw();
		UI::update();
		UI::draw();
		Window::display();
	}
	return 0;
}