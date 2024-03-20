#include "hScript.hpp"
#include "hGlobal.hpp"
#include "hWorld.hpp"
#include "hAssets.hpp"
#include "hWindow.hpp"
#include <iostream>
#include <deque>

#define special sf::String(" ;()[]{}+-=*/^<>&|!")

std::vector<Script::MathExpr> Script::math;
std::vector<Script::LogicExpr> Script::logic;

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

bool Script::Token::operator==(Script::Token t1)
{
	return type == t1.type && value == t1.value;
}

bool Script::Token::operator!=(Script::Token t1)
{
	return type != t1.type || value != t1.value;
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
		if (t[0].type == Token::Variable && t[1] == Token(Token::MathOperator, "="))
		{
			cmd.type = Command::Assign;
			t.erase(t.begin() + 1);
			cmd.args.push_back(t[0]);
			t.erase(t.begin());
			if (t[0].type == Token::String)
			{
				cmd.args.push_back(t[0]);
			}
			else
			{
				math.push_back(MathExpr(t));
				cmd.args.push_back({Token::MathExpression, std::to_string(math.size() - 1)});
			}
			commands.push_back(cmd);
		}
		if (t[0].type == Token::IfStatement && t[1] == Token(Token::Bracket, "("))
		{
			cmd.type = Command::Compare;
			int j = 2;
			std::vector<Token> expr;
			while (t[j] != Token(Token::Bracket, ")")) { expr.push_back(t[j++]); }
			logic.push_back(LogicExpr(expr));
			cmd.args.push_back({Token::LogicExpression, std::to_string(logic.size() - 1)});
			cmd.args.push_back({Token::String, t[t.size() - 2].value});
			cmd.args.push_back({Token::String, t[t.size() - 1].value});
			commands.push_back(cmd);
		}
		if (t[0].type == Token::WhileLoop && t[1] == Token(Token::Bracket, "("))
		{
			cmd.type = Command::Loop;
			int j = 2;
			std::vector<Token> expr;
			while (t[j] != Token(Token::Bracket, ")")) { expr.push_back(t[j++]); }
			logic.push_back(LogicExpr(expr));
			cmd.args.push_back({Token::LogicExpression, std::to_string(logic.size() - 1)});
			cmd.args.push_back({Token::String, t[t.size() - 1].value});
			commands.push_back(cmd);
		}
		if (t[0].type == Token::DeleteVar && t[1].type == Token::Variable)
		{
			cmd.type = Command::DeleteVar;
			cmd.args.push_back(t[1]);
			commands.push_back(cmd);
		}
		if (t[0].type == Token::Call && t[1].type == Token::String)
		{
			cmd.type = Command::CallFunction;
			cmd.args.push_back(t[1]);
			commands.push_back(cmd);
		}
		if (t[0].type == Token::Execute && (t[1].type == Token::String || t[1].type == Token::Variable))
		{
			cmd.type = Command::CallEngine;
			cmd.args.push_back(t[1]);
			commands.push_back(cmd);
		}
	}
}

void Script::Function::execute(Programmable *prog, Script *launcher)
{
	for (int i = 0; i < commands.size(); i++)
	{
		auto cmd = commands[i];
		if (cmd.type == Command::Assign)
		{
			auto target = tr::splitStr(cmd.args[0].value, ".");
			auto *t = getProg(target[0], prog);
			if (cmd.args[1].type == Token::String)
			{
				t->setVar(target[1], cmd.args[1].value);
			}
			else if (cmd.args[1].type == Token::MathExpression)
			{
				auto expr = std::stoi(cmd.args[1].value.toAnsiString());
				t->setVar(target[1], math[expr].eval(prog));
			}
		}
		else if (cmd.type == Command::Compare)
		{
			auto expr = std::stoi(cmd.args[0].value.toAnsiString());
			bool check = logic[expr].eval(prog);
			if (check) { if (cmd.args[1].value != "null") launcher->execute(cmd.args[1].value, prog); }
			else if (cmd.args[2].value != "null") { launcher->execute(cmd.args[2].value, prog); }
		}
		else if (cmd.type == Command::Loop)
		{
			auto expr = std::stoi(cmd.args[0].value.toAnsiString());
			while (logic[expr].eval(prog)) { launcher->execute(cmd.args[1].value, prog); }
		}
		else if (cmd.type == Command::DeleteVar)
		{
			auto target = tr::splitStr(cmd.args[0].value, ".");
			getProg(target[0], prog)->deleteVar(target[1]);
		}
		else if (cmd.type == Command::CallFunction)
		{
			launcher->execute(cmd.args[0].value, launcher);
		}
		else if (cmd.type == Command::CallEngine)
		{
			if (cmd.args[0].type == Token::Variable)
			{
				auto var = tr::splitStr(cmd.args[0].value, ".");
				tr::execute(getProg(var[0], prog)->getVar(var[1]).str);
			}
			else tr::execute(cmd.args[0].value);
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
	std::string word;
	std::vector<Script::Token> tokens;
	for (int i = 0; i < code.getSize(); i++)
	{
		std::string ch; ch += code.toAnsiString()[i];
		if (ch == "\"")
		{
			ch = "";
			i++;
			while (ch != "\"")
			{
				word += ch;
				ch = code.toAnsiString()[i++];
			}
			i--;
			continue;
		}
		else if (!tr::strContains(special, ch)) { word += ch; }
		else
		{
			auto cur = convert(ch), next = convert(code.toAnsiString()[i + 1]);
			if ((cur.type == Token::MathOperator || cur.type == Token::LogicOperator) &&
				(next.type == Token::MathOperator || next.type == Token::LogicOperator)) { ch += code.toAnsiString()[++i]; }
			if (cur == Token(Token::MathOperator, "-") && next.type == Token::Number) { word += ch; continue; }
			if (cur.type == Token::MathOperator && next.type == Token::String) { word += ch; continue; }
			if (cur == Token(Token::String, "!") && next == Token(Token::MathOperator, "=")) { tokens.push_back(convert("!=")); continue; }
			if (cur == Token(Token::MathOperator, "=") && next == Token(Token::MathOperator, "=")) { tokens.push_back(convert("==")); continue; }
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
	if (value.isEmpty() || value == " ") { return {Token::Invalid, ""}; }
	if (value == "function") { return {Token::Function, ""}; }
	if (value == "if") { return {Token::IfStatement, ""}; }
	if (value == "while") { return {Token::WhileLoop, ""}; }
	if (value == "delete") { return {Token::DeleteVar, ""}; }
	if (value == "call") { return {Token::Call, ""}; }
	if (value == "execute") { return {Token::Execute, ""}; }
	if (value == ";") { return {Token::Semicolon, ""}; }
	if (tr::strContains(sf::String("()[]{}"), value)) { return {Token::Bracket, value}; }
	if (tr::strContains(sf::String("+-=*/^"), value) && value.getSize() == 1) { return {Token::MathOperator, value}; }
	if (value == "<" || value == "<=" ||
		value == "!=" || value == "==" ||
		value == ">" || value == ">=" ||
		value == "&&" || value == "&" ||
		value == "||" || value == "|") { return {Token::LogicOperator, value}; }
	if (tr::strContains(value, ".") && !std::isdigit(value.toAnsiString()[0])) { return {Token::Variable, value }; }
	if (!std::isdigit(value.toAnsiString()[0]) && value.toAnsiString()[0] != '-') { return {Token::String, value}; }
	else
	{
		bool ch = false;
		for (int i = 0; i < value.getSize(); i++)
		{
			if (tr::strContains(special, value[i])) { break; }
			if (!(std::isdigit(value.toAnsiString()[i]) || value.toAnsiString()[i] == '.' || value.toAnsiString()[i] == '-')) { ch = true; break; }
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
		if (funcs[i].name == funcName) { funcs[i].execute(executor, this); }
	}
}

Script::MathToken::MathToken()
{
	type = Invalid;
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
		else if (regTokens[i].type == Token::MathOperator) { t.push_back({MathToken::Operator, regTokens[i].value}); }
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
				if (src[0] == "math") { stack.push_back(evalMath(src[1], prog)); }
				else { stack.push_back(getProg(src[0], prog)->getVar(src[1])); }
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

Script::LogicToken::LogicToken()
{
	type = Invalid;
	value = "";
	priority = 0;
}

Script::LogicToken::LogicToken(Script::LogicToken::Type token, sf::String val)
{
	type = token;
	value = val;
	priority = 0;
	if (value == "<" || value == "<=" ||
		value == "==" || value == "!=" ||
		value == ">" || value == ">=") priority = 2;
	if (value == "&&" || value == "||") priority = 1;
}

Script::LogicExpr::LogicExpr() { tokens.clear(); }

Script::LogicExpr::LogicExpr(std::vector<Script::Token> t)
{
	tokens.clear();
	shuntingYard(t);
}

void Script::LogicExpr::shuntingYard(std::vector<Script::Token> regTokens)
{
	std::deque<LogicToken> t;
	for (int i = 0; i < regTokens.size(); i++)
	{
		if (regTokens[i].type == Token::Variable) { t.push_back({LogicToken::Variable, regTokens[i].value}); }
		else if (regTokens[i].type == Token::Number) { t.push_back({LogicToken::Number, regTokens[i].value}); }
		else if (regTokens[i].type == Token::LogicOperator) { t.push_back({LogicToken::Operator, regTokens[i].value}); }
		else if (regTokens[i].type == Token::Bracket)
		{
			if (regTokens[i].value == "(") { t.push_back({LogicToken::LeftBracket, ""}); }
			if (regTokens[i].value == ")") { t.push_back({LogicToken::RightBracket, ""}); }
		}
	}
	tokens.clear();
	std::vector<LogicToken> stack;

	for (auto token : t)
	{
		switch (token.type)
		{
			case LogicToken::Type::Number: tokens.push_back(token); break;
			case LogicToken::Type::Variable: tokens.push_back(token); break;
			case LogicToken::Type::Operator:
			{
				auto o1 = token;
				while (!stack.empty())
				{
					auto o2 = stack.back();
					if (o1.priority <= o2.priority)
					{
						stack.pop_back();
						tokens.push_back(o2);
						continue;
					}
					else break;
				}
				stack.push_back(o1);
			} break;
			case LogicToken::Type::LeftBracket: stack.push_back(token); break;
			case LogicToken::Type::RightBracket:
			{
				bool match = false;
				while (!stack.empty() && stack.back().type != LogicToken::Type::LeftBracket)
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
		if (stack.back().type == LogicToken::Type::LeftBracket) { return; }
		tokens.push_back(std::move(stack.back()));
		stack.pop_back();
	}
}

bool Script::LogicExpr::eval(Programmable *prog)
{
	std::vector<float> stack;
	std::deque<LogicToken> queue(tokens.begin(), tokens.end());

	while (!queue.empty())
	{
		auto token = queue.front();
		queue.pop_front();
		switch (token.type)
		{
			case LogicToken::Type::Number: stack.push_back(std::stof(token.value.toAnsiString())); break;
			case LogicToken::Type::Variable:
			{
				auto src = tr::splitStr(token.value, ".");
				stack.push_back(getProg(src[0], prog)->getVar(src[1]));
			}
			break;
			case LogicToken::Type::Operator:
				{
					auto rhs = stack.back(); stack.pop_back();
					auto lhs = stack.back(); stack.pop_back();

					auto type = token.value.toAnsiString();
					if (type == "<") { stack.push_back(lhs < rhs); }
					if (type == "<=") { stack.push_back(lhs <= rhs); }
					if (type == "==") { stack.push_back(lhs == rhs); }
					if (type == "!=") { stack.push_back(lhs != rhs); }
					if (type == ">=") { stack.push_back(lhs >= rhs); }
					if (type == ">") { stack.push_back(lhs > rhs); }
					if (type == "&&") { stack.push_back(lhs && rhs); }
					if (type == "||") { stack.push_back(lhs || rhs); }
				}
				break;
			default: break;
		}
	}

	return stack.back();
}

Programmable *Script::getProg(sf::String name, Programmable *def)
{
	if (name == "this") { return def; }
	if (name == "camOwner") { return World::getCameraOwner(); }
	return World::getCurrentLevel()->getEntity(name);
}

float Script::evalMath(sf::String func, Programmable *prog)
{
	if (func == "abs") { return abs(prog->getVar("arg1")); }
	if (func == "dist")
	{
		float dx = abs(prog->getVar("arg1") - prog->getVar("arg3"));
		float dy = abs(prog->getVar("arg2") - prog->getVar("arg4"));
		return sqrt(dx * dx + dy * dy);
	}
	if (func == "intersection")
	{
		sf::FloatRect r1(
			prog->getVar("x"),
			prog->getVar("y"),
			prog->getVar("w"),
			prog->getVar("h"));
		sf::FloatRect r2;
		if (prog->getVar("arg1").str == "camOwner") r2 = World::getCameraOwner()->getHitbox();
		else r2 = World::getCurrentLevel()->getEntity(prog->getVar("arg1"))->getHitbox();
		return r1.intersects(r2);
	}
	if (func == "deltaTime")
	{
		return Window::getDeltaTime();
	}
	return 0;
}