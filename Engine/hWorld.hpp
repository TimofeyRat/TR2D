#ifndef trWorld
#define trWorld

#include "hEntity.hpp"

#include <SFML/Audio.hpp>
#include <box2d/box2d.h>
#include <string>
#include <vector>

class World
{
public:
	struct Trigger : public Programmable
	{
		sf::FloatRect rect;
		Rigidbody rb;
		Trigger();
		Trigger(sf::FloatRect r, sf::String cmd);
	};
private:
	static std::vector<Entity> ents;
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
		void draw(sf::RenderTarget *target);
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
	struct Light
	{
		sf::Vector2f pos;
		float dist;
		sf::Color clr;
		Light();
		Light(sf::Vector2f position, float distance, sf::Color color);
	};
	struct FallenItem
	{
		Inventory::ItemEntry item;
		Rigidbody rb;
		sf::Clock cooldown;
		sf::Vector2f impulse;
		FallenItem();
		FallenItem(Inventory::ItemEntry entry, sf::Vector2f throwStart, sf::Vector2f impulse);
		void draw();
	};
	struct Control
	{
		sf::String entName, ctrlID;
		Control();
		Control(sf::String ctrl, sf::String ent);
	};
	struct Level
	{
		Map map;
		std::vector<Trigger> triggers;
		std::vector<Entity> ents;
		std::vector<SoundPlayer> sounds;
		std::vector<Light> lights;
		std::vector<FallenItem> items;
		std::vector<Control> controls;
		sf::String name;
		sf::Texture *bgTex;
		sf::Sprite bgSpr;
		b2Vec2 gravity;
		Camera cam;
		std::string musicFilename;
		Level();
		~Level();
		void reset();
		void update();
		void draw(sf::RenderTarget *target);
		Entity *getEntity(sf::String name);
	};
	static sf::Music music;
	static std::vector<Level> lvls;
	static int currentLevel;
	static sf::RenderTexture screen;
	static bool active;
public:
	static b2World *world;
	static void init();
	static void loadFromFile(std::string filename);
	static void update();
	static void draw();
	static sf::RenderTexture *getScreen();
	static Entity *getEnt(sf::String name);
	static void setActive(bool a);
	static bool getActive();
	static void playSound(std::string file, sf::Vector2f pos, float distance);
	static std::vector<Trigger*> getTriggers();
	static std::vector<Entity*> getEntsWithVar(sf::String name, sf::String value);
	static std::vector<Entity*> getEntsWithVar(sf::String name, float value);
	static Entity *getCameraOwner();
	static void throwItem(Inventory::Item itm, Entity *sender);
};

#endif