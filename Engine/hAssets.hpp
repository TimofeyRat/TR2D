#ifndef trAssets
#define trAssets

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>

//Get access to files in assets directory.
struct AssetManager
{
public:
	//Get all mods in game folder.
	static std::vector<sf::String> getMods();
	
	//Read file without using the AssetManager.
	static std::vector<sf::String> readFile(std::string path);
	
	//Initialize all the assets.
	// \param assets Path to directory with textures
	static void init();

	//Clear the assets.
	static void reset();
	
	//Change or add a Texture asset.
	// \param path Path to texture as if it would be in "res" directory
	// \param img New texture
	static void set(std::string path, sf::Image img);
	
	//Change or add a Texture asset.
	// \param path Path to texture as if it would be in "res" directory
	// \param tex New texture
	static void set(std::string path, sf::Texture tex);
	
	//Change or add a Text asset.
	// \param path Path to text as if it would be in "res" directory
	// \param text New text
	static void set(std::string path, sf::String text);
	
	//Change or add a Font asset.
	// \param path Path to font as if it would be in "res" directory
	// \param font New font
	static void set(std::string path, sf::Font font);

	//Change or add a Sound asset
	// \param path Path to sound as if it would be in "res" directory
	// \param sound New sound buffer
	static void set(std::string path, sf::SoundBuffer sound);

	//Get the Texture asset.
	// \param path Path to texture as if it would be in "res" directory
	static sf::Texture *getTexture(std::string path);

	//Get the Text asset.
	// \param path Path to text as if it would be in "res" directory
	static sf::String getText(std::string path);

	//Get the Font asset.
	// \param path Path to text as if it would be in "res" directory
	static sf::Font *getFont(std::string path);

	//Get the Sound asset
	// \param path Path to sound as if it would be in "res" directory
	static sf::SoundBuffer *getSound(std::string path);

	//Get all the textures paths that contain some part.
	// \param part Part of path that every texture path has to contain
	static std::vector<std::string> getTextures(std::string part);
	
	//Get all the texts paths that contain some part.
	// \param part Part of path that every text path has to contain
	static std::vector<std::string> getTexts(std::string part);
	
	//Get all the fonts paths that contain some part.
	// \param part Part of path that every font path has to contain
	static std::vector<std::string> getFonts(std::string part);

	//Get all the sounds paths that contain some part.
	// \param part Part of path that every sound path has to contain
	static std::vector<std::string> getSounds(std::string part);

	//Request to reload a texture.
	// \param path Path to texture as if it would be in "res" directory
	static void reloadTexture(std::string path);
	
	//Request to reload a text.
	// \param path Path to text as if it would be in "res" directory
	static void reloadText(std::string path);
	
	//Request to reload a font.
	// \param path Path to font as if it would be in "res" directory
	static void reloadFont(std::string path);
	
	//Request to reload a sound.
	// \param path Path to sound as if it would be in "res" directory
	static void reloadSound(std::string path);
	static sf::String path;
private:
	struct Texture
	{
		sf::Texture tex;
		std::string path;
		Texture();
		Texture(std::string file);
		Texture(sf::Image img, std::string file);
		Texture(sf::Texture img, std::string file);
	};
	struct Text
	{
		std::string path;
		sf::String text;
		Text();
		Text(std::string file);
		Text(sf::String txt, std::string file);
	};
	struct Font
	{
		std::string path;
		sf::Font font;
		Font();
		Font(std::string file);
		Font(sf::Font font, std::string file);
	};
	struct Sound
	{
		std::string path;
		sf::SoundBuffer music;
		Sound();
		Sound(std::string path);
		Sound(sf::SoundBuffer sb, std::string file);
	};
	static std::vector<std::string> iterateDir(std::string dir);
	static std::vector<Texture> textures;
	static std::vector<Text> texts;
	static std::vector<Font> fonts;
	static std::vector<Sound> sounds;
	//Loading animation
	static short currentFileLoading;
	static void load();
};

#endif