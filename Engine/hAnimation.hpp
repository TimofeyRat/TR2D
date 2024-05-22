#ifndef trAnimation
#define trAnimation

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

#include <pugixml.hpp>

class FrameAnimator
{
public:
	struct Animation
	{
		std::vector<sf::IntRect> frames, frames_flip;
		float duration, currentFrame, scaleX, scaleY;
		bool repeat;
		sf::Texture *texture;
		sf::String name;
		Animation();
	};
	FrameAnimator();
	void loadFromFile(std::string filename);
	void setCurrentAnimation(sf::String currentAnim);
	void setCurrentAnimation(sf::Uint16 currentAnim);
	void update();
	void send(sf::Sprite &spr, bool flip = false, bool scale = true);
	Animation *getCurrentAnim();
	Animation *getAnim(sf::String name);
	void restart();
	sf::IntRect getCurrentFrame(bool flip);
private:
	std::vector<Animation> anims;
	sf::Uint16 currentAnimation;
};

class Skeleton
{
private:
	struct Texture
	{
		sf::Texture *tex;
		sf::String name;
		sf::IntRect rect;
		Texture();
		Texture(sf::String id, sf::String path, sf::IntRect tr);
	};
	struct Bone
	{
		int root, layer;
		float length, angle;
		sf::Vector2f pos;
		sf::Sprite spr;
		sf::Vector3f angle_origin;
		Bone();
		Bone(int Root,
			float Length,
			sf::Vector3f ao = {0, 0, 0},
			int Layer = 0);
		sf::Vector2f getEnd(float scale);
	};
	struct Animation
	{
		struct Frame
		{
			float r;
			sf::String tex;
			int layer, root;
			float timestamp;
			sf::Vector2f origin;
			void init();
			Frame();
			Frame(pugi::xml_node node);
			void parse(pugi::xml_node node);
		};
		struct Changer
		{
			int boneID;
			std::vector<Frame> frames;
			Changer();
			Changer(pugi::xml_node node);
			void parse(pugi::xml_node node);
			Frame *getFrame(float timestamp);
			Frame *getNextFrame(float timestamp);
		};
		std::vector<Changer> changes;
		sf::String name;
		float currentFrame, duration;
		bool repeat;
		void init();
		Animation();
		Animation(pugi::xml_node node);
		void parse(pugi::xml_node node);
	};
	std::vector<Texture> tex;
	std::vector<Bone> bones;
	std::vector<Animation> anims;
	int currentAnim;
	sf::Vector2f position;
	sf::Color color;
	float speed, scale;
public:
	Skeleton();
	Skeleton(std::string filename);
	void init();
	void loadFromFile(std::string filename);
	void updateBones();
	void update();
	void draw(sf::RenderTarget *target, const sf::RenderStates &states = sf::RenderStates::Default);
	void drawLayer(int layer, sf::RenderTarget *target, const sf::RenderStates &states = sf::RenderStates::Default);
	void drawBones(sf::RenderTarget *target);
	Bone *getBone(int ID);
	Texture *getTexture(sf::String name);
	void setPosition(sf::Vector2f xy);
	sf::Vector2f getPosition();
	void setCurrentAnimation(sf::String name);
	Animation *getCurrentAnim();
	void restart();
	void setColor(sf::Color clr);
	sf::Color getColor();
	void setSpeed(float s);
	float getSpeed();
	void setScale(float s);
	float getScale();
	bool hasAnimationEnded();
	sf::FloatRect generateHitbox();
	sf::Glsl::Vec4 getBonePoints(sf::Uint16 id);
	float getBoneAngle(sf::Uint16 id);
};

#endif