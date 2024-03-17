#include "hWindow.hpp"
#include "hGlobal.hpp"
#include "hAssets.hpp"

#include <pugixml.hpp>

sf::RenderWindow Window::window;
Programmable Window::vars;
sf::Clock Window::deltaTimer;
std::vector<sf::Event> Window::events;
sf::RenderTexture Window::screen;
float Window::deltaTime;

void Window::init(int argc, char *argv[])
{
	pugi::xml_document config;
	config.load_file("res/global/settings.trconf");
	for (auto set : config.child(L"settings").children())
	{
		vars.setVar(set.name(), set.attribute(L"num").as_float());
		vars.setVar(set.name(), set.attribute(L"str").as_string());
	}
	if (vars.getVar("Fullscreen"))
	{
		window.create(
			sf::VideoMode::getDesktopMode(),
			vars.getVar("Title").str,
			sf::Style::Fullscreen,
			sf::ContextSettings(24, 0, 0, 3, 3)
		);
	}
	else
	{
		window.create(
			sf::VideoMode(vars.getVar("SizeX"), vars.getVar("SizeY")),
			vars.getVar("Title").str,
			sf::Style::Default,
			sf::ContextSettings(24, 0, 0, 3, 3)
		);
		if (hasVar("PosX") && hasVar("PosY")) window.setPosition({getVar("PosX"), getVar("PosY")});
	}
	screen.create(getSize().x, getSize().y);
	window.setVerticalSyncEnabled(vars.getVar("VSync"));
	window.setActive();
	if (vars.hasVar("Icon"))
	{
		sf::String str = vars.getVar("Icon").str;
		sf::Image img; img.loadFromFile(str);
		window.setIcon(img.getSize().x, img.getSize().y, img.getPixelsPtr());
	}
	// window.setMouseCursorVisible(false);
	for (int i = 1; i < argc; i++)
	{
		auto args = tr::splitStr(argv[i], "=");
		if (args[1] == "str") { vars.setVar(args[0], args[2]); }
		else if (args[1] == "num") { vars.setVar(args[0], std::stof(args[2].toAnsiString())); }
	}
}

void Window::update()
{
	deltaTime = deltaTimer.restart().asSeconds();
	setVar("FPS", 1.0f / deltaTime);
	setVar("mouseX", sf::Mouse::getPosition(window).x);
	setVar("mouseY", sf::Mouse::getPosition(window).y);
	window.setVerticalSyncEnabled(getVar("VSync"));
	events.clear();
	sf::Event e;
	while (window.pollEvent(e))
	{
		if (e.type == sf::Event::Closed) { window.close(); }
		else if (e.type == sf::Event::Resized)
		{
			window.setView(sf::View({0, 0, e.size.width, e.size.height}));
			screen.create(e.size.width, e.size.height);
		}
		events.push_back(e);
	}
}

sf::Event Window::getEvent(sf::Event::EventType type)
{
	for (int i = 0; i < events.size(); i++) { if (events[i].type == type) { return events[i]; } }
	return sf::Event();
}

sf::Event Window::waitEvent(sf::Event::EventType type)
{
	sf::Event e;
	while (e.type != type)
	{
		Window::update();
		e = getEvent(type);
	}
	return e;
}

void Window::clear()
{
	auto clr = tr::splitStr(getVar("clearClr").str, "-");
	sf::Color clearClr = sf::Color::Black;
	if (clr.size() > 0) clearClr = sf::Color(
		std::stoi(clr[0].toAnsiString()), std::stoi(clr[1].toAnsiString()),
		std::stoi(clr[2].toAnsiString()), std::stoi(clr[3].toAnsiString())
	);
	window.clear(clearClr);
	screen.clear(clearClr);
}

sf::Texture Window::capture()
{
	sf::Texture t;
	t.update(window);
	return t;
}

void Window::draw(sf::Drawable &obj, bool drawUI, const sf::RenderStates states)
{
	if (drawUI) window.draw(obj, states);
	else screen.draw(obj, states);
}

void Window::drawScreen()
{
	screen.display();
	auto s = sf::Sprite(screen.getTexture());
	draw(s);
}

void Window::setView(sf::View view)
{
	screen.setView(view);
}

void Window::close() { window.close(); screen.clear(); }
void Window::display() { window.display(); }
bool Window::hasEvent(sf::Event::EventType type) { return getEvent(type).type == type; }
void Window::addEvent(sf::Event e) { events.push_back(e); }
float Window::getDeltaTime() { return deltaTime; }
bool Window::isOpen() { return window.isOpen(); }
void Window::resetTime() { deltaTime = 0; deltaTimer.restart(); }
sf::Vector2f Window::getSize() { return (sf::Vector2f)window.getSize(); }
bool Window::hasFocus() { return window.hasFocus(); }
void Window::clearVars() { vars.clear(); }
void Window::setVar(sf::String name, sf::String value) { vars.setVar(name, value); }
void Window::setVar(sf::String name, float value) { vars.setVar(name, value); }
Programmable::Variable Window::getVar(sf::String name) { return vars.getVar(name); }
bool Window::hasVar(sf::String name) { return vars.hasVar(name); }
sf::RenderTarget *Window::getRenderTarget() { return &window; }
sf::RenderTexture *Window::getScreen() { return &screen; }
Programmable *Window::getProgrammable() { return &vars; }

void Window::setTitle(sf::String title)
{
	window.setTitle(title);
	setVar("Title", title);
}

sf::String Window::getTitle()
{
	return getVar("Title");
}