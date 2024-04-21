#include "hGlobal.hpp"
#include "hProgrammable.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>


bool tr::strContains(sf::String str, sf::String obj)
{
	return str.find(obj) != sf::String::InvalidPos;
}

std::vector<sf::String> tr::splitStr(sf::String line, sf::String sep)
{
	std::vector<sf::String> parts;
    std::wstring wideInput = line.toWideString();
    std::wstring wideSeparator = sep.toWideString();
    std::wstringstream ss(wideInput);
    sf::String part;
    bool inQuotes = false;

    while (ss.good()) {
        std::wstring substr;
        std::getline(ss, substr, wideSeparator[0]);
        part = substr;

        if (!inQuotes) {
            parts.push_back(part);
            // Check if the part starts with a quote
            if (part.getSize() > 0 && part[0] == L'\"') {
                inQuotes = true;
            }
        }
        else {
            // Add the part to the last element in the vector rather than creating a new element
            parts.back() += sep + part;
            // Check if the part ends with a quote
            if (part.getSize() > 0 && part[part.getSize() - 1] == L'\"') {
                inQuotes = false;
            }
        }
    }
	for (int i = 0; i < parts.size(); i++)
	{
		if (parts[i].isEmpty()) { parts.erase(parts.begin() + i--); continue; }
		if (parts[i].toAnsiString()[0] == '"')
		{
			parts[i].erase(0);
			parts[i].erase(parts[i].find("\""));
		}
	}
    return parts;
}

sf::Vector2f tr::getDelta(float angle)
{
	return sf::Vector2f(cos(angle * DEGTORAD), sin(angle * DEGTORAD));
}

float tr::clamp(float x, float min, float max)
{
	return fmax(min, fmin(x, max));
}

float tr::lerp(float start, float end, float t)
{
	return start + (end - start) * t;
}

float tr::inverseLerp(float start, float end, float t)
{
	return start + ((end - start) * t);
}

sf::Vector2f tr::lerpVec(sf::Vector2f start, sf::Vector2f end, float t)
{
	//Not ready
	return sf::Vector2f(lerp(start.x, end.x, t), lerp(start.y, end.y, t));
}

sf::Vector2f tr::inverseLerpVec(sf::Vector2f start, sf::Vector2f end, float t)
{
	//Not ready
	return sf::Vector2f(
		inverseLerp(start.x, end.x, t),
		inverseLerp(start.y, end.y, t)
	);
}

sf::Vector2f tr::clampVec(sf::Vector2f x, sf::Vector2f min, sf::Vector2f max)
{
	return { tr::clamp(x.x, min.x, max.x), tr::clamp(x.y, min.y, max.y) };
}

sf::String tr::partsToLine(std::vector<sf::String> parts, sf::String sep)
{
	sf::String end;
	for (int i = 0; i < parts.size() - 1; i++) { end += parts[i] + sep; }
	end += parts[parts.size() - 1];
	return end;
}

sf::Glsl::Vec4 tr::lerpClr(sf::Glsl::Vec4 start, sf::Glsl::Vec4 end, float t)
{
	return {
		lerp(start.x, end.x, t), lerp(start.y, end.y, t),
		lerp(start.z, end.z, t), lerp(start.w, end.w, t)
	};
}

float tr::randBetween(float min, float max)
{
	return min + (float)rand() / RAND_MAX * (max - min);
}