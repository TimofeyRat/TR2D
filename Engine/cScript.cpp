#include "hScript.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"
#include "hAssets.hpp"
#include <iostream>
#include <deque>

std::vector<Script::MathExpr> Script::expressions;

Script::Token::Token()
{
	type = Type::Invalid;
	value = "";
}

Script::Token::Token(Script::Token::Type token, sf::String val)
{
	type = token;
	value = val;
}

Script::Command::Command()
{
	type = Invalid;
	args.clear();
}

Script::Command::Command(Script::Command::Type cmd, std::vector<Script::Token> tokens)
{
	type = cmd;
	args = tokens;
}

Script::Function::Function()
{
	name = "";
	commands.clear();
}

void Script::Function::parse(std::vector<Script::Token> tokens)
{
	std::vector<std::vector<Token>> t1(1);
	for (int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i].type == Token::Semicolon) { t1.push_back({}); continue; }
		t1[t1.size() - 1].push_back(tokens[i]);
	}
	for (int i = 0; i < t1.size(); i++)
	{
		Command cmd;
		auto t = t1[i];
		if (t.empty()) { continue; }
		if (t[0].type == Token::Variable &&
			t[1].type == Token::Operator && t[1].value == "=")
		{
			cmd.type = Command::Assign;
			t.erase(t.begin() + 1);
			cmd.args.push_back(t[0]);
			t.erase(t.begin());
			expressions.push_back(MathExpr(t));
			cmd.args.push_back({Token::Expression, std::to_string(expressions.size() - 1)});
			commands.push_back(cmd);
		}
	}
}

void Script::Function::execute(Programmable *targetProg)
{
	Programmable *prog = targetProg;
	for (int i = 0; i < commands.size(); i++)
	{
		auto cmd = commands[i];
		if (cmd.type == Command::Assign)
		{
			auto target = tr::splitStr(cmd.args[0].value, ".");
			if (target[0] != "this") { prog = World::getCurrentLevel()->getEntity(target[0]); }
			prog->setVar(target[1], expressions[std::stoi(cmd.args[1].value.toAnsiString())].eval(prog));
		}
	}
}

Script::Script()
{
	clear();
	funcs.clear();
}

Script::Script(std::string filename)
{
	clear();
	funcs.clear();
	loadFromFile(filename);
}

void Script::loadFromFile(std::string filename)
{
	funcs.clear();
	auto text = AssetManager::getText(filename);
	while (tr::strContains(text, "\t")) { text.erase(text.find("\t")); }
	while (tr::strContains(text, "\n")) { text.erase(text.find("\n")); }
	auto t = tokenize(text);
	Function func;
	for (int i = 0; i < t.size(); i++)
	{
		// std::cout << tokens[i].type << "|" << tokens[i].value.toAnsiString() << std::endl;
		if (t[i].type == Token::Type::Function)
		{
			if (t[i + 1].type == Token::String) { func.name = t[i + 1].value; }
			if (t[i + 2].type == Token::Bracket && t[i + 2].value == "{")
			{
				auto begin = t.begin() + i + 3;
				auto last = begin;
				for (int j = i + 3; j < t.size(); j++)
				{
					if (t[j].type == Token::Bracket && t[j].value == "}") last = t.begin() + j;
				}
				func.parse(std::vector<Script::Token>(begin, last));
			}
			funcs.push_back(func);
			func = Function();
		}
	}
}

std::vector<Script::Token> Script::tokenize(sf::String code)
{
	sf::String special = " ;()[]{}+-=*/";
	sf::String word;
	std::vector<Script::Token> tokens;
	for (int i = 0; i < code.getSize(); i++)
	{
		sf::String ch = code[i];
		if (!tr::strContains(special, ch))
		{
			word += ch;
		}
		else
		{
			auto t1 = convert(word), t2 = convert(ch);
			if (t1.type != Token::Invalid)
			{
				tokens.push_back(t1);
			}
			if (t2.type != Token::Invalid)
			{
				tokens.push_back(t2);
			}
			word.clear();
		}
	}
	return tokens;
}

Script::Token Script::convert(sf::String value)
{
	sf::String special = " ;()[]{}+-=*/";
	if (value.isEmpty() || value == " ") { return {Token::Invalid, ""}; }
	if (value == "function") { return {Token::Function, ""}; }
	if (value == ";") { return {Token::Semicolon, ""}; }
	if (tr::strContains(sf::String("()[]{}"), value)) { return {Token::Bracket, value}; }
	if (tr::strContains(sf::String("+-=*/"), value) && value.getSize() == 1) { return {Token::Operator, value}; }
	if (tr::strContains(value, ".") && !std::isdigit(value.toAnsiString()[0])) { return {Token::Variable, value }; }
	if (!std::isdigit(value.toAnsiString()[0]) && value.toAnsiString()[0] != '_') { return {Token::String, value}; }
	else
	{
		bool ch = false;
		for (int i = 0; i < value.getSize(); i++)
		{
			if (tr::strContains(special, value[i])) { break; }
			if (!(std::isdigit(value.toAnsiString()[i]) || value.toAnsiString()[i] == '.' || value.toAnsiString()[i] == '_')) { ch = true; break; }
		}
		if (!ch)
		{
			if (value.toAnsiString()[0] == '_') { value.erase(0); value.insert(0, "-"); }
			return {Token::Number, value};
		}
	}
	return {Token::Invalid, ""};
}

void Script::execute(sf::String funcName, Programmable *executor)
{
	if (executor == nullptr) { executor = this; }
	for (int i = 0; i < funcs.size(); i++)
	{
		if (funcs[i].name == funcName) { funcs[i].execute(executor); }
	}
}

Script::MathToken::MathToken()
{
	type = Unknown;
	value = "";
	priority = 0;
	rightAssociative = false;
}

Script::MathToken::MathToken(Script::MathToken::Type token, sf::String val)
{
	type = token;
	value = val;
	priority = 0;
	rightAssociative = false;
	if (tr::strContains(sf::String("/*"), value)) priority = 2;
	if (tr::strContains(sf::String("+-"), value)) priority = 1;
	if (value == "^") { priority = 3; rightAssociative = true; }
}

Script::MathExpr::MathExpr() { tokens.clear(); }

Script::MathExpr::MathExpr(std::vector<Script::Token> t)
{
	tokens.clear();
	shuntingYard(t);
}

void Script::MathExpr::shuntingYard(std::vector<Script::Token> regTokens)
{
	std::deque<MathToken> t;
	//Parse regular tokens to mathematical tokens
	for (int i = 0; i < regTokens.size(); i++)
	{
		if (regTokens[i].type == Token::Variable) { t.push_back({MathToken::Variable, regTokens[i].value}); }
		else if (regTokens[i].type == Token::Number) { t.push_back({MathToken::Number, regTokens[i].value}); }
		else if (regTokens[i].type == Token::Operator) { t.push_back({MathToken::Operator, regTokens[i].value}); }
		else if (regTokens[i].type == Token::Bracket)
		{
			if (regTokens[i].value == "(") { t.push_back({MathToken::LeftBracket, ""}); }
			if (regTokens[i].value == ")") { t.push_back({MathToken::RightBracket, ""}); }
		}
	}
	//Shunting Yard algorithm
	tokens.clear();
	std::vector<MathToken> stack;

	for (auto token : t)
	{
		switch (token.type)
		{
			case MathToken::Type::Number: tokens.push_back(token); break;
			case MathToken::Type::Variable: tokens.push_back(token); break;
			case MathToken::Type::Operator:
			{
				auto o1 = token;
				while (!stack.empty())
				{
					auto o2 = stack.back();
					if ((!o1.rightAssociative && o1.priority <= o2.priority) ||
						(o1.rightAssociative && o1.priority < o2.priority))
					{
						stack.pop_back();
						tokens.push_back(o2);
						continue;
					}
					else break;
				}
				stack.push_back(o1);
			} break;
			case MathToken::Type::LeftBracket: stack.push_back(token); break;
			case MathToken::Type::RightBracket:
			{
				bool match = false;
				while (!stack.empty() && stack.back().type != MathToken::Type::LeftBracket)
				{
					tokens.push_back(stack.back());
					stack.pop_back();
					match = true;
				}
				if (!match && stack.empty()) { return; }
			} break;
			default: break;
		}
	}

	while (!stack.empty())
	{
		if (stack.back().type == MathToken::Type::LeftBracket) { return; }
		tokens.push_back(std::move(stack.back()));
		stack.pop_back();
	}
}

float Script::MathExpr::eval(Programmable *prog)
{
	std::vector<float> stack;
	std::deque<MathToken> queue(tokens.begin(), tokens.end());

	while (!queue.empty())
	{
		auto token = queue.front();
		queue.pop_front();
		switch (token.type)
		{
			case MathToken::Type::Number: stack.push_back(std::stof(token.value.toAnsiString())); break;
			case MathToken::Type::Variable:
			{
				auto src = tr::splitStr(token.value, ".");
				if (src[0] != "this") stack.push_back(World::getCurrentLevel()->getEntity(src[0])->getVar(src[1]));
				else stack.push_back(prog->getVar(src[1]));
			}
			break;
			case MathToken::Type::Operator:
				{
					auto rhs = stack.back(); stack.pop_back();
					auto lhs = stack.back(); stack.pop_back();

					switch (token.value.toAnsiString()[0])
					{
						case '^': stack.push_back(pow(lhs, rhs)); break;
						case '*': stack.push_back(lhs * rhs); break;
						case '/': stack.push_back(lhs / rhs); break;
						case '+': stack.push_back(lhs + rhs); break;
						case '-': stack.push_back(lhs - rhs); break;
						default: break;
					}
				}
				break;
			default: break;
		}
	}

	return stack.back();
}