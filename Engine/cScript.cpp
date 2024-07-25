#include "hScript.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"
#include "hWindow.hpp"
#include "hInput.hpp"
#include <iostream>

Programmable* Script::currentExecutor;

Script::Script()
{
	started = false;
	state = luaL_newstate();
	luaL_openlibs(state);
	lua_register(state, "getNum", getNum);
	lua_register(state, "setNum", setNum);
	lua_register(state, "getStr", getStr);
	lua_register(state, "setStr", setStr);
	lua_register(state, "exec", exec);
	lua_register(state, "getDeltaTime", getDeltaTime);
	lua_register(state, "getExecNum", getExecutorNum);
	lua_register(state, "setExecNum", setExecutorNum);
	lua_register(state, "getExecStr", getExecutorStr);
	lua_register(state, "setExecStr", setExecutorStr);
	lua_register(state, "hasItem", hasItem);
	lua_register(state, "hasVar", hasVar);
	lua_register(state, "checkCollision", checkCollision);
}

int Script::getNum(lua_State *L)
{
	auto path = tr::splitStr(lua_tostring(L, -1), "-");
	float value = 0;
	if (path[0] == "input") value = Input::getControl(path[1])->getVariable(path[2])->value;
	else value = getProgrammable(path[0])->getVar(path[1]);
	lua_pushnumber(L, value);
	return 1;
}

int Script::setNum(lua_State *L)
{
	auto path = tr::splitStr(lua_tostring(L, -2), "-");
	getProgrammable(path[0])->setVar(path[1], lua_tonumber(L, -1));
	return 0;
}

int Script::getStr(lua_State *L)
{
	auto path = tr::splitStr(lua_tostring(L, -1), "-");
	lua_pushstring(L, getProgrammable(path[0])->getVar(path[1]).str.toAnsiString().c_str());
	return 1;
}

int Script::setStr(lua_State *L)
{
	auto path = tr::splitStr(lua_tostring(L, -2), "-");
	getProgrammable(path[0])->setVar(path[1], lua_tostring(L, -1));
	return 0;
}

int Script::exec(lua_State *L)
{
	tr::execute(lua_tostring(L, -1));
	return 0;
}

int Script::getExecutorNum(lua_State *L)
{
	lua_pushnumber(L, currentExecutor->getVar(lua_tostring(L, -1)));
	return 1;
}

int Script::setExecutorNum(lua_State *L)
{
	currentExecutor->setVar(lua_tostring(L, -2), lua_tonumber(L, -1));
	return 0;
}

int Script::getExecutorStr(lua_State *L)
{
	lua_pushstring(L, currentExecutor->getVar(lua_tostring(L, -1)).str.toAnsiString().c_str());
	return 1;
}

int Script::setExecutorStr(lua_State *L)
{
	currentExecutor->setVar(lua_tostring(L, -2), lua_tostring(L, -1));
	return 0;
}

int Script::getDeltaTime(lua_State *L) { lua_pushnumber(L, Window::getDeltaTime()); return 1; }

int Script::hasItem(lua_State *L) { lua_pushboolean(L, Inventory::hasItem(lua_tostring(L, -2), lua_tonumber(L, -1))); return 1; }

int Script::hasVar(lua_State *L)
{
	auto path = tr::splitStr(lua_tostring(L, -1), "-");
	lua_pushboolean(L, getProgrammable(path[0])->hasVar(path[1]));
	return 1;
}

int Script::checkCollision(lua_State *L)
{
	lua_pushboolean(L, World::checkCollision(lua_tostring(L, -1), lua_tostring(L, -2)));
	return 1;
}

void Script::load(sf::String path) { code = AssetManager::getText(path); }
void Script::execute(sf::String func)
{
	if (code.isEmpty()) { return; }
	int r = luaL_dostring(state, code.toAnsiString().c_str());
	if (r != LUA_OK)
	{
		std::cout << "Error executing Lua code:\n" << lua_tostring(state, -1) << std::endl;
		return;
	}
	if (!started)
	{
		lua_getglobal(state, "init");
		lua_call(state, 0, 0);
		started = true;
	}
	lua_getglobal(state, func.toAnsiString().c_str());
	lua_call(state, 0, 0);
}

Programmable *Script::getProgrammable(sf::String name)
{
	auto path = tr::splitStr(name, "-");
	if (path[0] == "Window") return Window::getProgrammable();
	if (path[0] == "camOwner") return World::getCameraOwner();
	if (path[0] == "lvl") return World::getCurrentLevel();
	if (path[0] == "input") return nullptr;
	if (auto ent = World::getCurrentLevel()->getEntity(path[0])) return ent;
	return World::getCurrentLevel()->getTrigger(path[0]);
}