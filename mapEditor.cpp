#include "Engine/hWindow.hpp"
#include "Engine/hWorld.hpp"

int main(int argc, char* argv[])
{
	Window::init(argc, argv);
	Window::setTitle("MapEditor");

	World::init();

	while (Window::isOpen())
	{
		Window::update();

		Window::clear();
		Window::display();
	}
	return 0;
}