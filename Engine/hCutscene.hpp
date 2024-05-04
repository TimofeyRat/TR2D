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
			struct Basis
			{
				float offset, ot;
				float duration, current;
				bool continueAfterEnd, ended;
				Basis(float os = 0, float d = 0, bool cae = false);
				void basisUpdate();
				bool isActive();
				void resetBasis(float os = 0, float d = 0, bool cae = false);
			};
			struct MoveEnt : public Basis
			{
				sf::String entName;
				std::vector<sf::Vector2f> curve;
				MoveEnt();
				void update();
			};
			struct MoveCam : public Basis
			{
				std::vector<sf::Vector2f> pos;
				sf::Vector2f startSize, endSize;
				sf::String sizeMath;
				MoveCam();
				void update();
			};
			struct AnimEnt : public Basis
			{
				sf::String entName, animName;
				float animSpeed;
				AnimEnt();
				void update();
			};
			struct Execute : public Basis
			{
				sf::String command;
				int count, executed;
				float timer, freq;
				Execute();
				void update();
				void execute();
			};
			struct CurveEnts : public Basis
			{
				sf::String entA, entB;
				bool created;
				std::vector<sf::Vector2f> curve;
				int count;
				sf::String partName, math;
				sf::Glsl::Vec4 speed;
				float lt, s, l;
				CurveEnts();
				void update();
			};
			bool startPhraseOnEnd, waitAll;
			std::vector<MoveCam> cam;
			std::vector<MoveEnt> moves;
			std::vector<AnimEnt> anims;
			std::vector<Execute> exec;
			std::vector<CurveEnts> cents;
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
	static bool active, shouldEnd;
	static sf::String getMusic(sf::String phrase);
	static void drawDebug(sf::RenderTarget *target);
	//x - type(0 - frame, 1 - world), y - index
	static sf::Vector2i current;
	static std::vector<FrameCutscene> frames;
	static std::vector<WorldCutscene> worlds;
};

#endif