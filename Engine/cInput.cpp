#include "hInput.hpp"
#include "hWindow.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"
#include "hAssets.hpp"

#include <pugixml.hpp>

#include <iostream>

std::vector<Input::Controller> Input::controls;
bool Input::active;
int Input::mainControl;

bool Input::isKeyPressed(sf::Keyboard::Key key)
{
	return sf::Keyboard::isKeyPressed(key) && Window::hasFocus();
}

bool Input::isKeyJustPressed(sf::Keyboard::Key key)
{
	return Window::getEvent(sf::Event::KeyPressed).key.code == key;
}

sf::Keyboard::Key Input::getPressedKey()
{
	return Window::waitEvent(sf::Event::KeyPressed).key.code;
}

bool Input::isMBPressed(sf::Mouse::Button btn)
{
	return sf::Mouse::isButtonPressed(btn) && Window::hasFocus();
}

bool Input::isMBJustPressed(sf::Mouse::Button btn)
{
	auto e = Window::getEvent(sf::Event::MouseButtonPressed);
	return e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == btn;
}

sf::Mouse::Button Input::getPressedButton()
{
	return Window::waitEvent(sf::Event::MouseButtonPressed).mouseButton.button;
}

bool Input::isJoyPressed(sf::Uint32 id, sf::Uint32 btn)
{
	auto vendor = getVendor(id);
	auto button = 0;
	if (vendor == xVendor)
	{
		switch (btn)
		{
			case jSouth: button = xA; break;
			case jNorth: button = xY; break;
			case jWest: button = xB; break;
			case jEast: button = xX; break;
			case jLB: button = xLB; break;
			case jRB: button = xRB; break;
			case jLT:
				return sf::Joystick::getAxisPosition(id, sf::Joystick::Z) > 5;
				break;
			case jRT:
				return sf::Joystick::getAxisPosition(id, sf::Joystick::Z) < -5;
				break;
			case jShare: button = xShare; break;
			case jOptions: button = xOptions; break;
			case jLS: button = xLS; break;
			case jRS: button = xRS; break;
			default: button = sf::Joystick::getButtonCount(id); break;
		}
	}
	if (vendor == dsVendor)
	{
		switch (btn)
		{
			case jSouth: button = dsCross; break;
			case jNorth: button = dsTriangle; break;
			case jWest: button = dsSquare; break;
			case jEast: button = dsCircle; break;
			case jLB: button = dsL1; break;
			case jRB: button = dsR1; break;
			case jLT: button = dsL2; break;
			case jRT: button = dsR2; break;
			case jShare: button = dsShare; break;
			case jOptions: button = dsOptions; break;
			case jLS: button = dsL3; break;
			case jRS: button = dsR3; break;
			default: button = sf::Joystick::getButtonCount(id); break;
		}
	}
	return sf::Joystick::isButtonPressed(id, button);
}

bool Input::isJoyJustPressed(sf::Uint32 id, sf::Uint32 btn)
{
	auto vendor = getVendor(id);
	auto button = sf::Joystick::getButtonCount(id);
	auto e1 = Window::getEvent(sf::Event::JoystickMoved).joystickMove;
	if (vendor == xVendor)
	{
		switch (btn)
		{
			case jSouth: button = xA; break;
			case jNorth: button = xY; break;
			case jWest: button = xX; break;
			case jEast: button = xB; break;
			case jLB: button = xLB; break;
			case jRB: button = xRB; break;
			case jLT:
				return e1.joystickId == id && e1.axis == sf::Joystick::Z && e1.position > 5;
				break;
			case jRT:
				return e1.joystickId == id && e1.axis == sf::Joystick::Z && e1.position < 5;
				break;
			case jShare: button = xShare; break;
			case jLS: button = xLS; break;
			case jRS: button = xRS; break;
		}
	}
	if (vendor == dsVendor)
	{
		switch (btn)
		{
			case jSouth: button = dsCross; break;
			case jNorth: button = dsTriangle; break;
			case jWest: button = dsSquare; break;
			case jEast: button = dsCircle; break;
			case jLB: button = dsL1; break;
			case jRB: button = dsR1; break;
			case jLT: button = dsL2; break;
			case jRT: button = dsR2; break;
		}
	}
	auto e = Window::getEvent(sf::Event::JoystickButtonPressed);
	return e.type == sf::Event::JoystickButtonPressed && e.joystickButton.joystickId == id && e.joystickButton.button == button;
}

sf::Vector2u Input::getPressedJoyButton()
{
	auto e = Window::waitEvent(sf::Event::JoystickButtonPressed).joystickButton;
	return sf::Vector2u(e.joystickId, e.button);
}

sf::Vector3f Input::getMovedJoyAxis()
{
	auto e = Window::waitEvent(sf::Event::JoystickMoved).joystickMove;
	return sf::Vector3f(e.joystickId, e.axis, e.position);
}

float Input::getJoyAxis(sf::Uint32 id, sf::Uint32 axis)
{
	auto vendor = getVendor(id);
	auto v = 0.0f;
	if (vendor == xVendor)
	{
		switch (axis)
		{
			case jLX:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::X);
				break;
			case jLY:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::Y);
				break;
			case jRX:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::U);
				break;
			case jRY:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::V);
				break;
			case jPovX:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::PovX);
				break;
			case jPovY:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::PovY) * -1;
				break;
		}
	}
	else if (vendor == dsVendor)
	{
		switch (axis)
		{
			case jLX:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::X);
				break;
			case jLY:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::Y);
				break;
			case jRX:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::Z);
				break;
			case jRY:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::R);
				break;
			case jPovX:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::PovX);
				break;
			case jPovY:
				v = sf::Joystick::getAxisPosition(id, sf::Joystick::PovY) * -1;
				break;
		}
	}
	if (v == tr::clamp(v, -5, 5)) { return 0; }
	return v / 100.0f;
}

int Input::getVendor(sf::Uint32 id)
{
	return sf::Joystick::getIdentification(id).vendorId;
}

bool Input::isJoyConnected(sf::Uint32 id)
{
	return sf::Joystick::isConnected(id);
}

sf::Vector2f Input::getMousePos(bool inGame)
{
	if (!inGame) return sf::Vector2f(
		Window::getVar("mouseX").num,
		Window::getVar("mouseY").num
	);
	else return Window::getScreen()->mapPixelToCoords({
		(int)Window::getVar("mouseX").num,
		(int)Window::getVar("mouseY").num
	});
}

sf::Uint32 Input::toUniversalButton(sf::Uint32 id, sf::Uint32 btn)
{
	auto vendor = getVendor(id);
	if (vendor == xVendor)
	{
		switch (btn)
		{
			case xA: return jSouth; break;
			case xB: return jEast; break;
			case xX: return jWest; break;
			case xY: return jNorth; break;
			case xLB: return jLB; break;
			case xRB: return jRB; break;
			case xShare: return jShare; break;
			case xOptions: return jOptions; break;
			case xLS: return jLS; break;
			case xRS: return jRS; break;
		}
	}
	if (vendor == dsVendor)
	{
		switch (btn)
		{
			case dsSquare: return jWest; break;
			case dsCross: return jSouth; break;
			case dsCircle: return jEast; break;
			case dsTriangle: return jNorth; break;
			case dsL1: return jLB; break;
			case dsR1: return jRB; break;
			case dsL2: return jLT; break;
			case dsR2: return jRT; break;
			case dsShare: return jShare; break;
			case dsOptions: return jOptions; break;
			case dsL3: return jLS; break;
			case dsR3: return jRS; break;
		}
	}
	return -1;
}

sf::Uint32 Input::toUniversalAxis(sf::Uint32 id, sf::Joystick::Axis axis)
{
	auto vendor = getVendor(id);
	if (vendor == xVendor)
	{
		switch (axis)
		{
			case sf::Joystick::Axis::X:
				return jLX;
				break;
			case sf::Joystick::Axis::Y:
				return jLY;
				break;
			case sf::Joystick::Axis::U:
				return jRX;
				break;
			case sf::Joystick::Axis::V:
				return jRY;
				break;
			case sf::Joystick::Axis::PovX:
				return jPovX;
				break;
			case sf::Joystick::Axis::PovY:
				return jPovY;
				break;
			default:
				return -1;
				break;
		}
	}
	else if (vendor == dsVendor)
	{
		switch (axis)
		{
			case sf::Joystick::Axis::X:
				return jLX;
				break;
			case sf::Joystick::Axis::Y:
				return jLY;
				break;
			case sf::Joystick::Axis::Z:
				return jRX;
				break;
			case sf::Joystick::Axis::R:
				return jRY;
				break;
			case sf::Joystick::Axis::PovX:
				return jPovX;
				break;
			case sf::Joystick::Axis::PovY:
				return jPovY;
				break;
		}
	}
	return -1;
}

sf::Keyboard::Key Input::strToKey(sf::String key)
{
	STR_TO_KEY(Unknown)
	STR_TO_KEY(A)
	STR_TO_KEY(B)
	STR_TO_KEY(C)
	STR_TO_KEY(D)
	STR_TO_KEY(E)
	STR_TO_KEY(F)
	STR_TO_KEY(G)
	STR_TO_KEY(H)
	STR_TO_KEY(I)
	STR_TO_KEY(J)
	STR_TO_KEY(K)
	STR_TO_KEY(L)
	STR_TO_KEY(M)
	STR_TO_KEY(N)
	STR_TO_KEY(O)
	STR_TO_KEY(P)
	STR_TO_KEY(Q)
	STR_TO_KEY(R)
	STR_TO_KEY(S)
	STR_TO_KEY(T)
	STR_TO_KEY(U)
	STR_TO_KEY(V)
	STR_TO_KEY(W)
	STR_TO_KEY(X)
	STR_TO_KEY(Y)
	STR_TO_KEY(Z)
	STR_TO_KEY(Num0)
	STR_TO_KEY(Num1)
	STR_TO_KEY(Num2)
	STR_TO_KEY(Num3)
	STR_TO_KEY(Num4)
	STR_TO_KEY(Num5)
	STR_TO_KEY(Num6)
	STR_TO_KEY(Num7)
	STR_TO_KEY(Num8)
	STR_TO_KEY(Num9)
	STR_TO_KEY(Escape)
	STR_TO_KEY(LControl)
	STR_TO_KEY(LShift)
	STR_TO_KEY(LAlt)
	STR_TO_KEY(LSystem)
	STR_TO_KEY(RControl)
	STR_TO_KEY(RShift)
	STR_TO_KEY(RAlt)
	STR_TO_KEY(RSystem)
	STR_TO_KEY(Menu)
	STR_TO_KEY(LBracket)
	STR_TO_KEY(RBracket)
	STR_TO_KEY(Semicolon)
	STR_TO_KEY(Comma)
	STR_TO_KEY(Period)
	STR_TO_KEY(Apostrophe)
	STR_TO_KEY(Slash)
	STR_TO_KEY(Backslash)
	STR_TO_KEY(Grave)
	STR_TO_KEY(Equal)
	STR_TO_KEY(Hyphen)
	STR_TO_KEY(Space)
	STR_TO_KEY(Enter)
	STR_TO_KEY(Backspace)
	STR_TO_KEY(Tab)
	STR_TO_KEY(PageUp)
	STR_TO_KEY(PageDown)
	STR_TO_KEY(End)
	STR_TO_KEY(Home)
	STR_TO_KEY(Insert)
	STR_TO_KEY(Delete)
	STR_TO_KEY(Add)
	STR_TO_KEY(Subtract)
	STR_TO_KEY(Multiply)
	STR_TO_KEY(Divide)
	STR_TO_KEY(Left)
	STR_TO_KEY(Right)
	STR_TO_KEY(Up)
	STR_TO_KEY(Down)
	STR_TO_KEY(Numpad0)
	STR_TO_KEY(Numpad1)
	STR_TO_KEY(Numpad2)
	STR_TO_KEY(Numpad3)
	STR_TO_KEY(Numpad4)
	STR_TO_KEY(Numpad5)
	STR_TO_KEY(Numpad6)
	STR_TO_KEY(Numpad7)
	STR_TO_KEY(Numpad8)
	STR_TO_KEY(Numpad9)
	STR_TO_KEY(F1)
	STR_TO_KEY(F2)
	STR_TO_KEY(F3)
	STR_TO_KEY(F4)
	STR_TO_KEY(F5)
	STR_TO_KEY(F6)
	STR_TO_KEY(F7)
	STR_TO_KEY(F8)
	STR_TO_KEY(F9)
	STR_TO_KEY(F10)
	STR_TO_KEY(F11)
	STR_TO_KEY(F12)
	STR_TO_KEY(F13)
	STR_TO_KEY(F14)
	STR_TO_KEY(F15)
	STR_TO_KEY(Pause)
	return sf::Keyboard::Unknown;
}

sf::Mouse::Button Input::strToMouse(sf::String btn)
{
	STR_TO_MOUSE(Left)
	STR_TO_MOUSE(Right)
	STR_TO_MOUSE(Middle)
	STR_TO_MOUSE(XButton1)
	STR_TO_MOUSE(XButton2)
	return sf::Mouse::XButton1;
}

sf::Uint32 Input::strToButton(sf::String btn)
{
	STR_TO_BUTTON(South)
	STR_TO_BUTTON(North)
	STR_TO_BUTTON(West)
	STR_TO_BUTTON(East)
	STR_TO_BUTTON(LB)
	STR_TO_BUTTON(RB)
	STR_TO_BUTTON(LT)
	STR_TO_BUTTON(RT)
	STR_TO_BUTTON(Share)
	STR_TO_BUTTON(Options)
	STR_TO_BUTTON(LS)
	STR_TO_BUTTON(RS)
	STR_TO_BUTTON(Unknown)
	return -1;
}

sf::Uint32 Input::strToAxis(sf::String axis)
{
	STR_TO_AXIS(LX);
	STR_TO_AXIS(LY);
	STR_TO_AXIS(RX);
	STR_TO_AXIS(RY);
	STR_TO_AXIS(PovX);
	STR_TO_AXIS(PovY);
	return -1;
}

sf::String Input::keyToStr(sf::Keyboard::Key key)
{
	KEY_TO_STR(Unknown)
	KEY_TO_STR(A)
	KEY_TO_STR(B)
	KEY_TO_STR(C)
	KEY_TO_STR(D)
	KEY_TO_STR(E)
	KEY_TO_STR(F)
	KEY_TO_STR(G)
	KEY_TO_STR(H)
	KEY_TO_STR(I)
	KEY_TO_STR(J)
	KEY_TO_STR(K)
	KEY_TO_STR(L)
	KEY_TO_STR(M)
	KEY_TO_STR(N)
	KEY_TO_STR(O)
	KEY_TO_STR(P)
	KEY_TO_STR(Q)
	KEY_TO_STR(R)
	KEY_TO_STR(S)
	KEY_TO_STR(T)
	KEY_TO_STR(U)
	KEY_TO_STR(V)
	KEY_TO_STR(W)
	KEY_TO_STR(X)
	KEY_TO_STR(Y)
	KEY_TO_STR(Z)
	KEY_TO_STR(Num0)
	KEY_TO_STR(Num1)
	KEY_TO_STR(Num2)
	KEY_TO_STR(Num3)
	KEY_TO_STR(Num4)
	KEY_TO_STR(Num5)
	KEY_TO_STR(Num6)
	KEY_TO_STR(Num7)
	KEY_TO_STR(Num8)
	KEY_TO_STR(Num9)
	KEY_TO_STR(Escape)
	KEY_TO_STR(LControl)
	KEY_TO_STR(LShift)
	KEY_TO_STR(LAlt)
	KEY_TO_STR(LSystem)
	KEY_TO_STR(RControl)
	KEY_TO_STR(RShift)
	KEY_TO_STR(RAlt)
	KEY_TO_STR(RSystem)
	KEY_TO_STR(Menu)
	KEY_TO_STR(LBracket)
	KEY_TO_STR(RBracket)
	KEY_TO_STR(Semicolon)
	KEY_TO_STR(Comma)
	KEY_TO_STR(Period)
	KEY_TO_STR(Apostrophe)
	KEY_TO_STR(Slash)
	KEY_TO_STR(Backslash)
	KEY_TO_STR(Grave)
	KEY_TO_STR(Equal)
	KEY_TO_STR(Hyphen)
	KEY_TO_STR(Space)
	KEY_TO_STR(Enter)
	KEY_TO_STR(Backspace)
	KEY_TO_STR(Tab)
	KEY_TO_STR(PageUp)
	KEY_TO_STR(PageDown)
	KEY_TO_STR(End)
	KEY_TO_STR(Home)
	KEY_TO_STR(Insert)
	KEY_TO_STR(Delete)
	KEY_TO_STR(Add)
	KEY_TO_STR(Subtract)
	KEY_TO_STR(Multiply)
	KEY_TO_STR(Divide)
	KEY_TO_STR(Left)
	KEY_TO_STR(Right)
	KEY_TO_STR(Up)
	KEY_TO_STR(Down)
	KEY_TO_STR(Numpad0)
	KEY_TO_STR(Numpad1)
	KEY_TO_STR(Numpad2)
	KEY_TO_STR(Numpad3)
	KEY_TO_STR(Numpad4)
	KEY_TO_STR(Numpad5)
	KEY_TO_STR(Numpad6)
	KEY_TO_STR(Numpad7)
	KEY_TO_STR(Numpad8)
	KEY_TO_STR(Numpad9)
	KEY_TO_STR(F1)
	KEY_TO_STR(F2)
	KEY_TO_STR(F3)
	KEY_TO_STR(F4)
	KEY_TO_STR(F5)
	KEY_TO_STR(F6)
	KEY_TO_STR(F7)
	KEY_TO_STR(F8)
	KEY_TO_STR(F9)
	KEY_TO_STR(F10)
	KEY_TO_STR(F11)
	KEY_TO_STR(F12)
	KEY_TO_STR(F13)
	KEY_TO_STR(F14)
	KEY_TO_STR(F15)
	KEY_TO_STR(Pause)
	return "Unknown";
}

sf::String Input::mouseToStr(sf::Mouse::Button btn)
{
	MOUSE_TO_STR(Left)
	MOUSE_TO_STR(Right)
	MOUSE_TO_STR(Middle)
	MOUSE_TO_STR(XButton1)
	MOUSE_TO_STR(XButton2)
	return "Unknown";
}

sf::String Input::btnToStr(sf::Uint32 btn)
{
	BUTTON_TO_STR(South)
	BUTTON_TO_STR(North)
	BUTTON_TO_STR(West)
	BUTTON_TO_STR(East)
	BUTTON_TO_STR(LB)
	BUTTON_TO_STR(RB)
	BUTTON_TO_STR(LT)
	BUTTON_TO_STR(RT)
	BUTTON_TO_STR(Share)
	BUTTON_TO_STR(Options)
	BUTTON_TO_STR(LS)
	BUTTON_TO_STR(RS)
	BUTTON_TO_STR(Unknown)
	return "Unknown";
}

sf::String Input::axisToStr(sf::Uint32 axis)
{
	AXIS_TO_STR(LX)
	AXIS_TO_STR(LY)
	AXIS_TO_STR(RX)
	AXIS_TO_STR(RY)
	AXIS_TO_STR(PovX)
	AXIS_TO_STR(PovY)
	return "Unknown";
}

/////////////
//Controllers
/////////////

Input::Controller::Variable::Variable()
{
	name = "";
	value = defValue = 0;
}

Input::Controller::Variable::Variable(sf::String Name)
{
	name = Name;
	value = defValue = 0;
}

Input::Controller::Variable::Variable(sf::String Name, float defaultValue)
{
	name = Name;
	value = defValue = defaultValue;
}

Input::Controller::Key::Key()
{
	key = varName = "";
	addOrSet = false;
	value = 0;
}

Input::Controller::Key::Key(sf::String Key, sf::String var, bool AddOrSet, float Value)
{
	key = Key;
	varName = var;
	addOrSet = AddOrSet;
	value = Value;
}

sf::String Input::Controller::Key::toString() { return tr::splitStr(key, "-")[2]; }

Input::Controller::Controller()
{
	id = "";
	keys.clear();
	vars.clear();
}

Input::Controller::Variable *Input::Controller::getVariable(sf::String name)
{
	for (int i = 0; i < vars.size(); i++)
	{
		if (vars[i].name == name) { return &vars[i]; }
	}
	return nullptr;
}

void Input::Controller::update()
{
	for (int i = 0; i < vars.size(); i++) { vars[i].value = vars[i].defValue; }
	if (!active) { return; }
	for (int i = 0; i < keys.size(); i++)
	{
		auto *k = &keys[i];
		auto *v = getVariable(k->varName);
		if (v == nullptr) { continue; }
		auto key = tr::splitStr(k->key, "-");
		bool active = false;
		if (key[0] == "key")
		{
			active = (key[1] == "press" && isKeyJustPressed(strToKey(key[2]))) || (key[1] == "hold" && isKeyPressed(strToKey(key[2])));
		}
		else if (key[0] == "mouse")
		{
			active = (key[1] == "press" && isMBJustPressed(strToMouse(key[2]))) || (key[1] == "hold" && isMBPressed(strToMouse(key[2])));
		}
		if (active)
		{
			if (!k->addOrSet) { v->value += k->value; }
			else { v->value = k->value; }
		}
	}
}

void Input::update() { for (int i = 0; i < controls.size(); i++) { controls[i].update(); } }

void Input::init()
{
	active = true;
	controls.clear();
	pugi::xml_document file;
	file.load_string(AssetManager::getText("res/global/settings.trconf").toWideString().c_str());
	for (auto controller : file.children())
	{
		if (sf::String(controller.name()) == "controller")
		{
			Controller ctrl;
			ctrl.id = controller.attribute(L"name").as_string();
			for (auto node : controller.children())
			{
				if (sf::String(node.name()) == "variable")
				{
					ctrl.vars.push_back({
						node.attribute(L"name").as_string(),
						node.attribute(L"default").as_float()
					});
				}
				else if (sf::String(node.name()) == "input")
				{
					bool addOrSet;
					if (sf::String(node.attribute(L"type").as_string()) == "add") { addOrSet = false; }
					if (sf::String(node.attribute(L"type").as_string()) == "set") { addOrSet = true; }
					ctrl.keys.push_back({
						node.attribute(L"event").as_string(),
						node.attribute(L"var").as_string(),
						addOrSet,
						node.attribute(L"value").as_float()
					});
				} 
			}
			controls.push_back(ctrl);
		}
	}
	for (int i = 0; i < controls.size(); i++)
	{
		for (int j = 0; j < controls[i].keys.size(); j++)
		{
			sf::String name = controls[i].keys[j].varName + "_key" + std::to_string(i);
			Window::setVar(name, controls[i].keys[j].key);
		}
	}
}

Input::Controller *Input::getControl(sf::String id)
{
	if (id == "main") { return &controls[mainControl]; }
	for (int i = 0; i < controls.size(); i++)
	{
		if (controls[i].id == id) { return &controls[i]; }
	}
	return nullptr;
}

Input::Controller::Key *Input::Controller::getKeyByVar(sf::String varName)
{
	for (int i = 0; i < keys.size(); i++)
	{
		if (keys[i].varName == varName) { return &keys[i]; }
	}
	return nullptr;
}