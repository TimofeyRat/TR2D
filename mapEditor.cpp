#include "Engine/hWindow.hpp"

int main(int argc, char* argv[])
{
	Window::init(argc, argv);

	while (Window::isOpen())
	{
		Window::update();

		Window::clear();
		Window::display();
	}
	return 0;
}