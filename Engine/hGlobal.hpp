#ifndef SFML_STATIC
#define SFML_STATIC
#endif

#ifndef trGlobal
#define trGlobal

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

class Programmable;

namespace tr
{
	const float PI = 3.141592, DEGTORAD = 0.017453f, M2P = 50.0f, RADTODEG=57.29577f;

	//Split the string into parts.
	// \param str String to be splitted
	// \param sep Separator
	std::vector<sf::String> splitStr(sf::String str, sf::String sep);

	//Check if string contains other string.
	// \param line String to be searched in
	// \param obj String to search in line
	bool strContains(sf::String line, sf::String obj);

	//Get the delta coordinates for an angle.
	// \param angle An angle to calculate the delta for(in degrees)
	sf::Vector2f getDelta(float angle);
	
	//Clamp the value.
	// \param x The value to clamp
	// \param min Minimal value of X
	// \param max Maximal value of X
	float clamp(float x, float min, float max);

	//Interpolate the delta between two values.
	// \param start The start value
	// \param end The end value
	// \param t Coefficient to interpolate value with
	float lerp(float start, float end, float t);

	//Inversed interpolation of delta between two values.
	// \param start The start value
	// \param end The end value
	// \param t Coefficient to interpolate value with
	float inverseLerp(float start, float end, float t);

	//Interpolate the delta between to vectors.
	// \param start The start vector
	// \param end The end vector
	// \param t Coefficient to interpolate vector with
	sf::Vector2f lerpVec(sf::Vector2f start, sf::Vector2f end, float t);

	//Inversed interpolation of delta between two vectors.
	// \param start The start vector
	// \param end The end vector
	// \param t Coefficient to interpolate vector with
	sf::Vector2f inverseLerpVec(sf::Vector2f start, sf::Vector2f end, float t);

	//Clamp the vector.
	// \param x The vector to clamp
	// \param min Minimal value of X
	// \param max Maximal value of X
	sf::Vector2f clampVec(sf::Vector2f x, sf::Vector2f min, sf::Vector2f max);

	//Connect parts to one string.
	// \param parts Parts to be connected
	// \param sep Separator to use between parts
	sf::String partsToLine(std::vector<sf::String> parts, sf::String sep);

	//Interpolate the delta between two vec4.
	// \param start The start vec4
	// \param end The end vec4
	// \param t Coefficient to interpolate vec4 with
	sf::Glsl::Vec4 lerpClr(sf::Glsl::Vec4 start, sf::Glsl::Vec4 end, float t);

	//Execute the engine-wide command.
	// \param cmd Command to execute
	// \warning If there are >=2 commands then you have to split them and execute one-by-one.
	void execute(sf::String cmd);
}

#endif