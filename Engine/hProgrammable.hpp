#ifndef trProgrammable
#define trProgrammable

#include <vector>
#include <SFML/System.hpp>

class Programmable
{
public:
	struct Variable
	{
		sf::String name, str;
		float num;
		Variable();
		Variable(sf::String Name, sf::String Value);
		Variable(sf::String Name, float Value);
		operator sf::String();
		operator float();
	};
	void clear();
	void setVar(sf::String name, sf::String value);
	void setVar(sf::String name, float value);
	void setVar(sf::String name, sf::String value, bool isNum);
	Variable getVar(sf::String name);
	bool hasVar(sf::String name);
	void deleteVar(sf::String name);
	const std::vector<Variable> getVars();
private:
	std::vector<Variable> vars;
};

#endif