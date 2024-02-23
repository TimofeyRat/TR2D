#ifndef trWindow
#define trWindow

#include <SFML/Graphics.hpp>
#include "hProgrammable.hpp"
#include <vector>

//Class for all the window stuff.
class Window
{
private:
	static sf::RenderWindow window;
	static sf::RenderTexture screen;
	static sf::Clock deltaTimer;
	static std::vector<sf::Event> events;
	static Programmable vars;
	static float deltaTime;
public:
	//Initialize the window.
	// \param argc Count of arguments provided while starting
	// \param argv Arguments provided while starting
	static void init(int argc, char *argv[]);

	//Check if window is open.
	static bool isOpen();

	//Update the window.
	static void update();

	//Reset the delta time(Time between frames).
	static void resetTime();

	//Get the delta time(Time between frames).
	static float getDeltaTime();

	//Get the event.
	// \param type Type of event to search for
	static sf::Event getEvent(sf::Event::EventType type);

	//Wait for event
	// \param type Type of event to wait for
	static sf::Event waitEvent(sf::Event::EventType type);

	//Check if event has happened.
	// \param type Type of event to search for
	static bool hasEvent(sf::Event::EventType type);

	//Add an event to event list.
	static void addEvent(sf::Event e);

	//Close the window.
	static void close();

	//Clear the window.
	static void clear();

	//Display the window graphics.
	static void display();

	//Get the size of window.
	static sf::Vector2f getSize();

	//Check if window has focus.
	static bool hasFocus();

	//Draw something on window.
	// \param obj Object to draw
	// \param states Parameters for drawing
	// \param states Render states(shader etc.)
	static void draw(sf::Drawable &obj, bool drawUI = true, const sf::RenderStates states = sf::RenderStates::Default);

	//Draw everything that is NOT the UI.
	static void drawScreen();

	//Set camera view
	static void setView(sf::View view);

	//Get the window as sf::RenderTarget to draw on.
	static sf::RenderTarget *getRenderTarget();

	//Get the screen that is drawing world.
	static sf::RenderTexture *getScreen();
	
	//Clear the variables.
	static void clearVars();

	//Change the value of/Add the variable.
	// \param name Name of variable
	// \param value New string value
	static void setVar(sf::String name, sf::String value);
	
	//Change the value of/Add the variable.
	// \param name Name of variable
	// \param value New float value
	static void setVar(sf::String name, float value);
	
	//Get the variable.
	// \param name Name of variable
	static Programmable::Variable getVar(sf::String name);
	
	//Check if variable exists.
	// \param name Name of variable to search for
	static bool hasVar(sf::String name);

	//Capture the contents of the window.
	static sf::Texture capture();

	//Get the Programmable part of the window.
	static Programmable *getProgrammable();

	//Change the title of the window.
	static void setTitle(sf::String title);

	//Get the title of the window.
	static sf::String getTitle();
};

#endif