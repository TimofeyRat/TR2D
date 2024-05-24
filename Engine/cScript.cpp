#include "hScript.hpp"
#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"
#include "hWindow.hpp"
#include <iostream>

Script::Script()
{
	started = false;
	state = luaL_newstate();
	luaL_openlibs(state);
	lua_register(state, "getNum", getNum);
	lua_register(state, "setNum", setNum);
	lua_register(state, "exec", exec);
	lua_register(state, "getDeltaTime", getDeltaTime);
}

int Script::getNum(lua_State *L)
{
	auto path = tr::splitStr(lua_tostring(L, -1), " ");
	float value = 0;
	if (path[0] == "ent")
	{
		value = World::getCurrentLevel()->getEntity(path[1])->getVar(path[2]);
	}
	if (path[0] == "lvl")
	{
		if (path[1] == "current") value = World::getCurrentLevel()->getVar(path[2]);
		else value = World::getLevel(path[1])->getVar(path[2]);
	}
	if (path[0] == "window")
	{
		value = Window::getVar(path[1]);
	}
	lua_pushnumber(L, value);
	return 1;
}

int Script::setNum(lua_State *L)
{
	auto path = tr::splitStr(lua_tostring(L, -1), " ");
	if (path[0] == "ent")
	{
		World::getCurrentLevel()->getEntity(path[1])->setVar(path[2], lua_tonumber(L, -2));
	}
	if (path[0] == "lvl")
	{
		if (path[1] == "current") World::getCurrentLevel()->setVar(path[2], lua_tonumber(L, -2));
		else World::getLevel(path[1])->setVar(path[2], lua_tonumber(L, -2));
	}
	return 0;
}

int Script::exec(lua_State *L)
{
	tr::execute(lua_tostring(L, -1));
	return 0;
}

int Script::getDeltaTime(lua_State *L) { lua_pushnumber(L, Window::getDeltaTime()); return 1; }

void Script::load(sf::String path) { code = AssetManager::getText(path); }
void Script::execute(sf::String func)
{
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