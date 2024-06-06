#ifndef trScript
#define trScript

#ifndef __WIN32__
extern "C" {
#endif
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#ifndef __WIN32__
}
#endif

#include "hProgrammable.hpp"

class Script
{
private:
	lua_State *state;
	sf::String code;
	bool started;
	static int getNum(lua_State *L);
	static int setNum(lua_State *L);
	static int setStr(lua_State *L);
	static int getStr(lua_State *L);
	static int exec(lua_State *L);
	static int getDeltaTime(lua_State *L);
	static int getExecutorNum(lua_State *L);
	static int setExecutorNum(lua_State *L);
	static int getExecutorStr(lua_State *L);
	static int setExecutorStr(lua_State *L);
public:
	static Programmable* currentExecutor;
	Script();
	void load(sf::String path);
	void execute(sf::String func);
	static Programmable *getProgrammable(sf::String path);
};

#endif