#ifndef trCutscene
#define trCutscene

#include <vector>
#include "hAnimation.hpp"

class CSManager
{
public:
	struct FrameCutscene
	{
		struct Change
		{
			sf::String phrase, anim;
			bool skippable;
			Change();
		};
		sf::String name, talkPath;
		FrameAnimator frame;
		std::vector<Change> changes;
		FrameCutscene();
		Change* getChange(sf::String phrase);
	};
	static void init();
	static void setCutscene(sf::String name);
	static void start();
	static sf::String getTalk();
	static bool active;
	//x - type(0 - frame, 1 - world), y - index
	static sf::Vector2i current;
	static std::vector<FrameCutscene> frames;
};

#endif