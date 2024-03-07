#ifndef trScript
#define trScript

#include "hProgrammable.hpp"

class Script : public Programmable
{
public:
	struct Token
	{
		enum Type
		{
			Invalid = -1,
			Function = 0,
			String = 1,
			Number = 2,
			Bracket = 3,
			Operator = 4,
			Semicolon = 5
		};
		Type type;
		sf::String value;
		Token();
		Token(Type token, sf::String val);
	};
	struct Function
	{
		sf::String name;
		std::vector<Token> tokens;
		Function();
		void execute(Programmable *prog);
	};
	Script();
	Script(std::string filename);
	void loadFromFile(std::string filename);
	void execute(sf::String funcName, Programmable *executor);
	static std::vector<Token> tokenize(sf::String code);
	static Token convert(sf::String value);
private:
	std::vector<Function> funcs;
};

#endif