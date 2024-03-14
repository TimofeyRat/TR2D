#ifndef hDialogue
#define hDialogue

#include <vector>
#include <string>
#include <SFML/Graphics.hpp>
#include <pugixml.hpp>

class Talk
{
public:
	struct SpeakerColor
	{
		sf::String name;
		sf::Color clr;
		SpeakerColor();
	};
	struct Dialogue
	{
		struct Phrase
		{
			struct Reply
			{
				std::vector<sf::String> actions;
				sf::String name, text, condition;
				Reply();
				Reply(sf::String N, sf::String T, sf::String C = "");
			};
			std::vector<Reply> replies;
			sf::String name, text, speaker;
			Phrase();
			Phrase(sf::String N, sf::String T, sf::String S);
		};
		std::vector<Phrase> phrases;
		sf::String name, currentPhrase;
		Dialogue();
		Dialogue(sf::String N);
		Phrase *getCurrentPhrase();
	};
	static std::vector<Dialogue> dialogues;
	static std::vector<SpeakerColor> nameColors;
	static sf::String currentDialogue;
	static bool active;
	static void init();
	static void loadFromFile(std::string filename);
	static Dialogue *getCurrentDialogue();
	static bool conditionCheck(sf::String condition);
	static void restart();
	static void loadNameColors();
	static sf::Color getNameColor(sf::String name);
};

#endif