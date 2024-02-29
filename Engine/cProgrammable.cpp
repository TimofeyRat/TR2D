#include "hProgrammable.hpp"

Programmable::Variable::Variable()
{
	name = str = "";
	num = 0;
}

Programmable::Variable::Variable(sf::String Name, sf::String Value)
{
	name = Name;
	str = Value;
	num = 0;
}

Programmable::Variable::Variable(sf::String Name, float Value)
{
	name = Name;
	str = "";
	num = Value;
}

Programmable::Variable::operator sf::String() { return str; }
Programmable::Variable::operator float() { return num; }

void Programmable::clear() { vars.clear(); }

void Programmable::setVar(sf::String name, sf::String value)
{
	bool found = false;
	for (int i = 0; i < vars.size(); i++)
	{
		if (vars[i].name == name) { vars[i].str = value; found = true; }
	}
	if (!found)
	{
		vars.push_back(Variable(name, value));
	}
}

void Programmable::setVar(sf::String name, float value)
{
	bool found = false;
	for (int i = 0; i < vars.size(); i++)
	{
		if (vars[i].name == name) { vars[i].num = value; found = true; }
	}
	if (!found)
	{
		vars.push_back(Variable(name, value));
	}
}

Programmable::Variable Programmable::getVar(sf::String name)
{
	for (int i = 0; i < vars.size(); i++)
	{
		if (vars[i].name == name) { return vars[i]; }
	}
	return Variable();
}

bool Programmable::hasVar(sf::String name)
{
	for (int i = 0; i < vars.size(); i++)
	{
		if (vars[i].name == name) return true;
	}
	return false;
}

const std::vector<Programmable::Variable> Programmable::getVars()
{
	return vars;
}