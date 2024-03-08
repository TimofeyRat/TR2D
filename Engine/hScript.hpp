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
			Semicolon = 5,
			Variable = 6,
			Expression = 7
		};
		Type type;
		sf::String value;
		Token();
		Token(Type token, sf::String val);
	};
	struct Command
	{
		enum Type
		{
			Invalid = -1,
			Assign = 0
		};
		Type type;
		std::vector<Token> args;
		Command();
		Command(Type cmd, std::vector<Token> tokens);
	};
	struct Function
	{
		sf::String name;
		std::vector<Command> commands;
		Function();
		void execute(Programmable *prog);
		void parse(std::vector<Token> tokens);
	};
	Script();
	Script(std::string filename);
	void loadFromFile(std::string filename);
	void execute(sf::String funcName, Programmable *executor);
private:
	struct MathToken
	{
		enum Type
		{
			Unknown = -1,
			Variable = 0,
			Number = 1,
			Operator = 2,
			LeftBracket = 3,
			RightBracket = 4
		};
		Type type;
		sf::String value;
		int priority;
		bool rightAssociative;
		MathToken();
		MathToken(Type token, sf::String val);
	};
	struct MathExpr
	{
		std::vector<MathToken> tokens;
		MathExpr();
		MathExpr(std::vector<Token> tokens);
		void shuntingYard(std::vector<Token> tokens);
		float eval(Programmable *prog);
	};
	static std::vector<MathExpr> expressions;
	static std::vector<Token> tokenize(sf::String code);
	static Token convert(sf::String value);
	std::vector<Function> funcs;
};

#endif