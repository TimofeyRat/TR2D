#ifndef trInput
#define trInput

#include <SFML/Window.hpp>
#include <vector>

#define STR_TO_KEY(x) if (key == #x) return sf::Keyboard::x;
#define STR_TO_MOUSE(x) if (btn == #x) return sf::Mouse::x;
#define STR_TO_BUTTON(x) if (btn == #x) return j##x;
#define STR_TO_AXIS(x) if (axis == #x) return j##x;
#define KEY_TO_STR(x) if (key == sf::Keyboard::x) return #x;
#define MOUSE_TO_STR(x) if (btn == sf::Mouse::x) return #x;
#define BUTTON_TO_STR(x) if (btn == j##x) return #x;
#define AXIS_TO_STR(x) if (axis == j##x) return #x;

const sf::Uint32 xVendor = 1118;
const sf::Uint32 xA = 0;
const sf::Uint32 xX = 1;
const sf::Uint32 xB = 2;
const sf::Uint32 xY = 3;
const sf::Uint32 xLB = 4;
const sf::Uint32 xRB = 5;
const sf::Uint32 xShare = 6;
const sf::Uint32 xOptions = 7;
const sf::Uint32 xLS = 8;
const sf::Uint32 xRS = 9;

const sf::Uint32 dsVendor = 1356;
const sf::Uint32 dsSquare = 0;
const sf::Uint32 dsCross = 1;
const sf::Uint32 dsCircle = 2;
const sf::Uint32 dsTriangle = 3;
const sf::Uint32 dsL1 = 4;
const sf::Uint32 dsR1 = 5;
const sf::Uint32 dsL2 = 6;
const sf::Uint32 dsR2 = 7;
const sf::Uint32 dsShare = 8;
const sf::Uint32 dsOptions = 9;
const sf::Uint32 dsL3 = 10;
const sf::Uint32 dsR3 = 11;

const sf::Uint32 jSouth = 0;
const sf::Uint32 jNorth = 1;
const sf::Uint32 jWest = 2;
const sf::Uint32 jEast = 3;
const sf::Uint32 jLB = 4;
const sf::Uint32 jRB = 5;
const sf::Uint32 jLT = 6;
const sf::Uint32 jRT = 7;
const sf::Uint32 jShare = 8;
const sf::Uint32 jOptions = 9;
const sf::Uint32 jLS = 10;
const sf::Uint32 jRS = 11;
const sf::Uint32 jUnknown = 12;
const sf::Uint32 jLX = 13;
const sf::Uint32 jLY = 14;
const sf::Uint32 jRX = 15;
const sf::Uint32 jRY = 16;
const sf::Uint32 jPovX = 17;
const sf::Uint32 jPovY = 18;

class Input
{
public:
	struct Controller
	{
		struct Variable
		{
			sf::String name;
			float value, defValue;
			Variable();
			Variable(sf::String Name);
			Variable(sf::String Name, float defaultValue);
		};
		struct Key
		{
			sf::String key, varName;
			bool addOrSet;
			float value;
			Key();
			Key(sf::String Key, sf::String var, bool AddOrSet, float Value);
			sf::String toString();
		};
		sf::String id;
		std::vector<Key> keys;
		std::vector<Variable> vars;
		Controller();
		Variable *getVariable(sf::String name);
		Key *getKeyByVar(sf::String varName);
		void update();
	};
	static std::vector<Controller> controls;
	static Controller *getControl(sf::String id);
	static bool active;
	static int mainControl;
	static void init();
	static void update();

	static bool isKeyPressed(sf::Keyboard::Key key);
	static bool isKeyJustPressed(sf::Keyboard::Key key);
	static bool isMBPressed(sf::Mouse::Button btn);
	static bool isMBJustPressed(sf::Mouse::Button btn);
	static bool isJoyPressed(sf::Uint32 id, sf::Uint32 btn);
	static bool isJoyJustPressed(sf::Uint32 id, sf::Uint32 btn);
	static float getJoyAxis(sf::Uint32 id, sf::Uint32 axis);
	static int getVendor(sf::Uint32 id);
	static bool isJoyConnected(sf::Uint32 id);
	static sf::Vector2f getMousePos(bool inGame = false);
	static sf::Keyboard::Key getPressedKey();
	static sf::Mouse::Button getPressedButton();
	static sf::Vector2u getPressedJoyButton();
	static sf::Vector3f getMovedJoyAxis();
	static sf::Uint32 toUniversalButton(sf::Uint32 id, sf::Uint32 btn);
	static sf::Uint32 toUniversalAxis(sf::Uint32 id, sf::Joystick::Axis axis);
	static sf::Keyboard::Key strToKey(sf::String key);
	static sf::Mouse::Button strToMouse(sf::String btn);
	static sf::Uint32 strToButton(sf::String btn);
	static sf::Uint32 strToAxis(sf::String axis);
	static sf::String keyToStr(sf::Keyboard::Key key);
	static sf::String mouseToStr(sf::Mouse::Button btn);
	static sf::String btnToStr(sf::Uint32 btn);
	static sf::String axisToStr(sf::Uint32 axis);
};

#endif