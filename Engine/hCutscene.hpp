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
	struct WorldCutscene
	{
		struct Change
		{
			struct MoveEnt
			{
				sf::String entName, math;
				sf::Vector2f start, target;
				MoveEnt();
				MoveEnt(sf::String name, sf::Vector2f s, sf::Vector2f e, sf::String lerpFunc);
			};
			struct MoveCam
			{
				sf::Vector2f startPos, endPos;
				sf::Vector2f startSize, endSize;
				sf::String posMath, sizeMath;
				MoveCam();
				MoveCam(sf::Glsl::Vec4 pos, sf::Glsl::Vec4 size, sf::String posLerpFunc, sf::String sizeLerpFunc);
			};
			struct AnimEnt
			{
				sf::String entName, animName;
				AnimEnt();
				AnimEnt(sf::String ent, sf::String anim);
			};
			MoveCam cam;
			bool startPhraseOnEnd;
			std::vector<MoveEnt> moves;
			std::vector<AnimEnt> anims;
			sf::String name, musicPath;
			float duration, current;
			Change();
			Change(sf::String name, sf::String music, bool spoe, float len);
			void update();
		};
		sf::String name, talkPath, currentChange;
		std::vector<Change> changes;
		WorldCutscene();
		WorldCutscene(sf::String n, sf::String talk);
		Change *getCurrentChange();
		Change *getChange(sf::String name);
		void update();
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
	static std::vector<WorldCutscene> worlds;
};

#endif