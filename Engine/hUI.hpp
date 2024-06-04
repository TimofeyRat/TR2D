#include "hAnimation.hpp"
#include "hProgrammable.hpp"

#include <pugixml.hpp>

class UI
{
public:
	struct Frame
	{
		struct Object
		{
			struct Action
			{
				sf::String event, condition, action;
				Action();
				Action(sf::String E, sf::String C, sf::String A);
				void execute();
				bool isActivated();
				bool isAvailable(bool objActive);
			};
			struct Sprite : public Programmable
			{
				bool active;
				sf::Sprite spr;
				FrameAnimator fa;
				std::vector<Action> actions;
				Sprite();
				Sprite(pugi::xml_node node);
				void update();
				void parse(pugi::xml_node node);
			};
			struct Text : public Programmable
			{
				bool active;
				sf::Text txt;
				sf::Font *font;
				sf::String idleTxt, activeTxt;
				sf::Glsl::Vec4 activeClr, idleClr, activeOut, idleOut;
				sf::Glsl::Vec4 curClr, curOut;
				std::vector<Action> actions;
				Text();
				Text(pugi::xml_node node);
				void update();
				void updateSize();
				void updateClr();
				void updateText();
				sf::String getText(sf::String appear, float speed);
				void parse(pugi::xml_node node);
			};
			struct Progress : public Programmable
			{
				bool active;
				float value, min, max;
				sf::Sprite bg, fg;
				FrameAnimator fa_bg, fa_fg;
				sf::Vector2f offset;
				sf::String target;
				Progress();
				Progress(pugi::xml_node node);
				void update();
				void parse(pugi::xml_node node);
			};
			std::vector<Sprite> sprites;
			std::vector<Text> texts;
			std::vector<Progress> bars;
			std::vector<Action> actions;
			sf::String name, toggle, pos, handler;
			sf::Vector2f position;
			bool active;
			Object();
			Object(sf::String N, bool A = false);
			void update();
			void draw();
			Sprite *getSprite(sf::String name);
			Text *getText(sf::String name);
			Progress *getProgress(sf::String name);
			void handle();
		};
		std::vector<Object> objects;
		Object *getObject(sf::String name);
		Object *getObjectByHandler(sf::String handler);
		sf::String name;
		Frame();
		Frame(sf::String N);
		void update();
		void draw();
	};
	static bool reloadUI;
	static std::vector<Frame> frames;
	static sf::String currentFrame, currentFile;
	static void init();
	static void loadFromFile(std::string filename, bool reload = false);
	static Frame *getCurrentFrame();
	static void update();
	static void draw();
	static sf::Vector2f getObjectPosition(sf::String var, bool active, sf::Vector2f old);
	static sf::Vector2f getObjectOrigin(sf::Vector2f size, sf::String var, bool active, sf::Vector2f old);
	static sf::Vector2f getObjectScale(sf::Vector2f size, sf::String var, bool active, sf::Vector2f curScale);
	static bool containsMouse();
	static bool containsMouse(Frame::Object *obj);
	static void setFrame(sf::String frame);
	static Frame *getFrame(sf::String name);
	static bool updateToggle(bool active, sf::String toggle, sf::FloatRect hitbox);
	static sf::String parseText(sf::String txt);
};