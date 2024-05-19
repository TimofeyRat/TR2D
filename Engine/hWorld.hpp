#ifndef trWorld
#define trWorld

#include "hEntity.hpp"
#include "hScript.hpp"
#include "hParticles.hpp"

#include <SFML/Audio.hpp>
#include <pugixml.hpp>
#include <box2d/box2d.h>
#include <string>
#include <vector>


class WorldCL : public b2ContactListener
{
public:
	WorldCL();
	void BeginContact(b2Contact *contact);
	void EndContact(b2Contact *contact);
private:
	void ParticleEntity(int particleID, sf::String entName, bool start = true);
	void ParticleTrigger(int particleID, sf::String triggerName, bool start = true);
	void EntityTrigger(sf::String entName, sf::String triggerName, bool start = true);
};

class World
{
public:
	struct Trigger : public Programmable
	{
		sf::FloatRect rect;
		Rigidbody rb;
		Trigger();
		Trigger(sf::String cmd);
	};
	struct Map
	{
		sf::Vector2i mapSize;
		std::vector<std::vector<sf::Uint16>> tiles;
		sf::Texture *tileTex;
		sf::Sprite tile;
		std::vector<sf::IntRect> tileRects;
		float scale;
		sf::Vector2i tileSize;
		std::string texFilename;
		Map();
		void reset();
		void draw(sf::RenderTarget *target, const sf::RenderStates &states = sf::RenderStates::Default);
		void resize(int x, int y);
		sf::Vector2f getPixelSize();
		void computeRects()
		{
			tileRects.clear();
			const auto texX = tileTex->getSize().x / tileSize.x,
				texY = tileTex->getSize().y / tileSize.y;
			for (short id = 0; id < texX * texY; id++)
			{
				sf::IntRect r;
				r.left = (int)(id % texX) * tileSize.x;
				r.top = (int)(id / texY) * tileSize.y;
				r.width = tileSize.x;
				r.height = tileSize.y;
				tileRects.push_back(r);
			}
		}
	};
	struct Camera
	{
		sf::View view;
		sf::String owner;
		sf::Vector2f offset;
		bool doUpdate;
		Camera();
		Camera(sf::Vector2f size, sf::Vector2f os, sf::String name);
		void update(sf::Vector2f mapSize);
	};
	struct SoundPlayer
	{
		sf::Sound sound;
		SoundPlayer();
		SoundPlayer(std::string file, sf::Vector2f pos, float distance);
	};
	struct FallenItem
	{
		Inventory::ItemEntry item;
		Rigidbody rb;
		sf::Clock cooldown;
		sf::Vector2f impulse;
		FallenItem();
		FallenItem(b2World *w, Inventory::ItemEntry entry, sf::Vector2f throwStart, sf::Vector2f impulse);
		void draw(b2World *world, sf::RenderTarget *target);
	};
	struct Control
	{
		sf::String entName, ctrlID;
		Control();
		Control(sf::String ctrl, sf::String ent);
	};
	struct Spawner
	{
		sf::String name;
		sf::Vector2f pos;
		Spawner();
		Spawner(sf::String ent, sf::Vector2f xy);
	};
	struct ScriptObject
	{
		Script src;
		sf::String mainFunc;
		sf::String executor;
		ScriptObject();
		ScriptObject(std::string filename, sf::String MAIN, sf::String owner);
	};
	struct ParticleGenerator
	{
		sf::String temp;
		sf::FloatRect spawnRect;
		sf::String velType, spawnRule;
		sf::Vector2f minVel, maxVel;
		float lifeTime, timer;
		ParticleGenerator();
		ParticleGenerator(sf::String name, sf::String spawn, sf::String vel, sf::Vector2f min, sf::Vector2f max, float timer, sf::FloatRect rect);
	};
	struct ParticleCurve : public Programmable
	{
		enum Type { TwoEnts, Standalone} type;
		enum Math { Set, FadeIn, FadeLerp, Pulse } math;
		std::vector<sf::Vector2f> curve;
		std::vector<Particle> parts;
		ParticleCurve();
		void init(b2World *w, Type t, sf::String entA, sf::String entB, std::vector<sf::Vector2f> points, float count, sf::String name, sf::Glsl::Vec4 speed, float lifeTime, float s, float length, float d);
		void join();
		void update();
		void draw(sf::RenderTarget *target);
	};
	struct LightSource
	{
		sf::Color color;
		sf::Vector2f position;
		float radius, angle, field;
		LightSource();
		LightSource(sf::Color clr, sf::Vector2f pos, float r, float a, float f);
	};
	struct Level : public Programmable
	{
		Map map;
		std::vector<Trigger> triggers;
		std::vector<Entity> ents;
		std::vector<Spawner> spawners;
		std::vector<SoundPlayer> sounds;
		std::vector<FallenItem> items;
		std::vector<Control> controls;
		std::vector<ScriptObject> scripts;
		std::vector<ParticleGenerator> partGens;
		std::vector<Particle> parts;
		std::vector<ParticleCurve> partCurves;
		std::vector<LightSource> lights;
		sf::String name;
		sf::Texture *bgTex;
		sf::Sprite bgSpr;
		b2Vec2 gravity;
		Camera cam;
		std::string musicFilename;
		sf::FloatRect bgBounds;
		bool started;
		b2World *world;
		float musicVolume;
		WorldCL cl;
		sf::Texture bg, lightMap;
		sf::RenderTexture *objects, *entsLayer;
		Level();
		~Level();
		void reset();
		void update();
		void draw();
		Entity *getEntity(sf::String name);
		Trigger *getTrigger(sf::String name);
	};
	static void init();
	static void loadFromFile(std::string filename);
	static void update();
	static void draw();
	static sf::RenderTexture *getScreen();
	static sf::String getEntFile(sf::String name);
	static void setActive(bool a);
	static bool getActive();
	static void playSound(std::string file, sf::Vector2f pos, float distance);
	static std::vector<Trigger*> getTriggers();
	static Entity *getCameraOwner();
	static void throwItem(Inventory::Item itm, Entity *sender);
	static Level *getCurrentLevel();
	static Level *getLevel(sf::String name);
	static void setCurrentLevel(sf::String name);
	static void saveGame(pugi::xml_node node);
	static void loadGame(pugi::xml_node node);
#ifndef trMapEditor
private:
#endif
	static std::map<sf::String, sf::String> ents;
	static sf::Music music;
	static std::vector<Level> lvls;
	static int currentLevel, nextLevel;
	static sf::RenderTexture screen;
	static float brightness, musicVolume;
	static bool active;
	static sf::String currentMusic, currentFile;
	static sf::Shader mapShader, lightShader, objectsShader, entShader;
};

#endif