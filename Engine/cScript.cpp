#include "hScript.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"
#include <iostream>

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

Script::Function::Function()
{
	name = "";
	tokens.clear();
}

void Script::Function::execute(Programmable *prog)
{
	//TODO
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
	auto tokens = tokenize(text);
	for (int i = 0; i < tokens.size(); i++)
	{
		std::cout << tokens[i].type << "|" << tokens[i].value.toAnsiString() << std::endl;
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
	if (tr::strContains(sf::String("+-=*/"), value)) { return {Token::Operator, value}; }
	if (!std::isdigit(value.toAnsiString()[0])) { return {Token::String, value}; }
	else
	{
		bool ch = false;
		for (int i = 0; i < value.getSize(); i++)
		{
			if (tr::strContains(special, value[i])) { break; }
			if (!(std::isdigit(value.toAnsiString()[i]) || value.toAnsiString()[i] == '.')) { ch = true; break; }
		}
		if (!ch) { return {Token::Number, value}; }
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