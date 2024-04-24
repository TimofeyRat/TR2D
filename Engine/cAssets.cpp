#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

std::vector<AssetManager::Texture> AssetManager::textures;
std::vector<AssetManager::Text> AssetManager::texts;
std::vector<AssetManager::Font> AssetManager::fonts;
std::vector<AssetManager::Sound> AssetManager::sounds;
short AssetManager::currentFileLoading;

std::vector<std::string> AssetManager::iterateDir(std::string dir)
{
	std::vector<std::string> files;
	for (const auto& p : std::filesystem::recursive_directory_iterator(dir))
	{
		if (p.is_regular_file())
		{
			auto path = p.path().string();
			while (path.find("\\") != std::string::npos) path.replace(path.find("\\"), 1, "/");
			files.push_back(path);
		}
	}
	return files;
}

std::vector<sf::String> AssetManager::readFile(std::string path)
{
	std::vector<sf::String> file;
	std::wifstream stream(path);
	stream.imbue(std::locale(stream.getloc(), new std::codecvt_utf8<wchar_t>));
	while (!stream.eof())
	{
		std::wstring line;
		std::getline(stream, line);
		if (!line.empty()) file.push_back(line);
	}
	if (!stream.is_open()) { return file; }
	return file;
}

AssetManager::Texture::Texture()
{
	tex = sf::Texture();
	path = "";
}
AssetManager::Texture::Texture(std::string file)
{
	tex.loadFromFile(file);
	path = file;
}
AssetManager::Texture::Texture(sf::Image img, std::string file)
{
	tex.loadFromImage(img);
	path = file;
}
AssetManager::Texture::Texture(sf::Texture img, std::string file)
{
	tex = img;
	path = file;
}
AssetManager::Text::Text()
{
	path = text = "";
}
AssetManager::Text::Text(std::string file)
{
	path = file;
	text = "";
	for (auto line : readFile(path))
	{
		text += line + L"\n";
	}
}
AssetManager::Text::Text(sf::String txt, std::string file)
{
	text = txt;
	path = file;
}
AssetManager::Font::Font()
{
	font = sf::Font();
	path = "";
}
AssetManager::Font::Font(std::string file)
{
	path = file;
	font.loadFromFile(path);
}
AssetManager::Font::Font(sf::Font f, std::string file)
{
	path = file;
	font = f;
}

void AssetManager::init()
{
	sf::Thread thread(&load);
	thread.launch();
	sf::Font f; f.loadFromFile("res/global/font.ttf");
	sf::Text t("", f, 32);
	float clr = 0;
	auto files = iterateDir("res");
	auto fileCount = files.size();
	while (currentFileLoading < fileCount)
	{
		Window::update();
		if (!Window::isOpen()) { exit(0); }

		clr = tr::clamp(clr + Window::getDeltaTime() * 200, 0, 255);

		t.setFillColor({clr, clr, clr, clr});
		t.setString("Loading resource " +
			std::to_string(currentFileLoading) + "/" + std::to_string(fileCount) +
			" (" + files[tr::clamp(currentFileLoading, 0, fileCount - 1)] + ")...");
		t.setOrigin(t.getGlobalBounds().getSize() / 2.0f);
		t.setPosition(Window::getSize() / 2.0f);

		Window::clear();
		Window::draw(t);
		Window::display();
	}
}

void AssetManager::load()
{
	reset();
	auto files = iterateDir("res");
	auto fileCount = files.size();
	currentFileLoading = 0;
	for (auto file : files)
	{
		if (tr::strContains(file, ".tr"))
		{
			auto txt = readFile(file);
			if (!txt.empty()) texts.push_back(Text(
				tr::partsToLine(txt, "\n"),
				file
			));
		}
		else if (tr::strContains(file, ".ttf"))
		{
			fonts.push_back(Font(file));
		}
		else if (tr::strContains(file, ".png"))
		{
			textures.push_back(Texture(file));
		}
		else if (tr::strContains(file, ".ogg"))
		{
			sounds.push_back(Sound(file));
		}
		currentFileLoading++;
	}
}

void AssetManager::reset()
{
	textures.clear();
	texts.clear();
	fonts.clear();
	sounds.clear();
}

void AssetManager::set(std::string path, sf::Image img)
{
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == path)
		{
			textures[i].tex.loadFromImage(img);
			return;
		}
	}
	Texture t;
	t.path = path;
	t.tex.loadFromImage(img);
	textures.push_back(t);
}

void AssetManager::set(std::string path, sf::Texture img)
{
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == path)
		{
			textures[i].tex = img;
			return;
		}
	}
	textures.push_back(Texture(img, path));
}

void AssetManager::set(std::string path, sf::String text)
{
	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i].path == path)
		{
			texts[i].text = text;
			return;
		}
	}
	texts.push_back(Text(text, path));
}

void AssetManager::set(std::string path, sf::Font font)
{
	for (int i = 0; i < fonts.size(); i++)
	{
		if (fonts[i].path == path) { fonts[i].font = font; return; }
	}
	fonts.push_back(Font(font, path));
}

void AssetManager::set(std::string path, sf::SoundBuffer sb)
{
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].path == path) { sounds[i].music = sb; return; }
	}
	sounds.push_back(Sound(sb, path));
}

sf::Texture *AssetManager::getTexture(std::string path)
{
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == path)
		{
			return &textures[i].tex;
		}
	}
	textures.push_back(Texture(path));
	return &textures[textures.size() - 1].tex;
}

sf::String AssetManager::getText(std::string path)
{
	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i].path == path)
		{
			return texts[i].text;
		}
	}
	texts.push_back(Text(path));
	return texts[texts.size() - 1].text;
}

sf::Font *AssetManager::getFont(std::string path)
{
	for (int i = 0; i < fonts.size(); i++)
	{
		if (fonts[i].path == path)
		{
			return &fonts[i].font;
		}
	}
	fonts.push_back(Font(path));
	return &fonts[fonts.size() - 1].font;
}

sf::SoundBuffer *AssetManager::getSound(std::string path)
{
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].path == path)
		{
			return &sounds[i].music;
		}
	}
	sounds.push_back(Sound(path));
	return &sounds[sounds.size() - 1].music;
}

std::vector<std::string> AssetManager::getTextures(std::string part)
{
	std::vector<std::string> paths;
	for (int i = 0; i < textures.size(); i++)
	{
		if (tr::strContains(textures[i].path, part))
		{
			paths.push_back(textures[i].path);
		}
	}
	return paths;
}

std::vector<std::string> AssetManager::getTexts(std::string part)
{
	std::vector<std::string> paths;
	for (int i = 0; i < texts.size(); i++)
	{
		if (tr::strContains(texts[i].path, part))
		{
			paths.push_back(texts[i].path);
		}
	}
	return paths;
}

std::vector<std::string> AssetManager::getFonts(std::string part)
{
	std::vector<std::string> paths;
	for (int i = 0; i < fonts.size(); i++)
	{
		if (tr::strContains(fonts[i].path, part))
		{
			paths.push_back(fonts[i].path);
		}
	}
	return paths;
}

std::vector<std::string> AssetManager::getSounds(std::string part)
{
	std::vector<std::string> paths;
	for (int i = 0; i < sounds.size(); i++)
	{
		if (tr::strContains(sounds[i].path, part))
		{
			paths.push_back(sounds[i].path);
		}
	}
	return paths;
}

AssetManager::Sound::Sound()
{
	path = "";
	music = sf::SoundBuffer();
}

AssetManager::Sound::Sound(std::string file)
{
	path = file;
	music.loadFromFile(path);
}

AssetManager::Sound::Sound(sf::SoundBuffer sb, std::string file)
{
	path = file;
	music = sb;
}

void AssetManager::reloadTexture(std::string path)
{
	auto tex = Texture(path);
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == path)
		{
			textures[i] = tex;
			return;
		}
	}
	textures.push_back(tex);
}

void AssetManager::reloadText(std::string path)
{
	auto text = Text(path);
	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i].path == path)
		{
			texts[i] = text;
			return;
		}
	}
	texts.push_back(text);
}

void AssetManager::reloadFont(std::string path)
{
	auto font = Font(path);
	for (int i = 0; i < fonts.size(); i++)
	{
		if (fonts[i].path == path)
		{
			fonts[i] = font;
			return;
		}
	}
	fonts.push_back(font);
}

void AssetManager::reloadSound(std::string path)
{
	auto sound = Sound(path);
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].path == path)
		{
			sounds[i] = sound;
			return;
		}
	}
	sounds.push_back(sound);
}