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
				sf::String entName;
				float duration, offset;
				std::vector<sf::Vector2f> curve;
				MoveEnt();
				MoveEnt(sf::String name, std::vector<sf::Vector2f> points, float s);
			};
			struct MoveCam
			{
				std::vector<sf::Vector2f> curve;
				sf::Vector2f startSize, endSize;
				sf::String sizeMath;
				float posDuration, offset;
				MoveCam();
				MoveCam(std::vector<sf::Vector2f> points, sf::Glsl::Vec4 size, sf::String sizeLerpFunc, float posSpeed);
			};
			struct AnimEnt
			{
				sf::String entName, animName;
				bool stopOnEnd;
				float offset;
				AnimEnt();
				AnimEnt(sf::String ent, sf::String anim, bool soe);
			};
			struct Execute
			{
				sf::String command;
				int count, exec;
				float timer, freq, offset;
				Execute();
				Execute(sf::String cmd, int count, float frequency);
			};
			MoveCam cam;
			bool startPhraseOnEnd;
			std::vector<MoveEnt> moves;
			std::vector<AnimEnt> anims;
			std::vector<Execute> exec;
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
	static void drawDebug(sf::RenderTarget *target);
	//x - type(0 - frame, 1 - world), y - index
	static sf::Vector2i current;
	static std::vector<FrameCutscene> frames;
	static std::vector<WorldCutscene> worlds;
};

#endif