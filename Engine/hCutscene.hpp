#ifndef trCutscene
#define trCutscene

#include <vector>
#include "hAnimation.hpp"
#include <SFML/Audio.hpp>

class CSManager
{
public:
	struct FrameCutscene
	{
		struct Change
		{
			sf::String phrase, anim, musicPath;
			bool skippable;
			Change();
		};
		sf::String name, talkPath;
		FrameAnimator frame;
		std::vector<Change> changes;
		FrameCutscene();
		Change* getChange(sf::String phrase);
	};
	static sf::Music music;
	static void init();
	static void setCutscene(sf::String name);
	static sf::String getTalk();
	static bool active;
	static sf::String getMusic(sf::String phrase);
	//x - type(0 - frame, 1 - world), y - index
	static sf::Vector2i current;
	static std::vector<FrameCutscene> frames;
};

#endif