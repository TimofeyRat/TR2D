#include "hAssets.hpp"
#include "hGlobal.hpp"
#include "hWindow.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <pugixml.hpp>
#include "hInput.hpp"

sf::String AssetManager::path;
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
	if (!tex.loadFromFile(file)) throw tr::AssetException(file);
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
	if (text.isEmpty()) throw tr::AssetException(file);
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
	if (!font.loadFromFile(path)) throw tr::AssetException(file);
}
AssetManager::Font::Font(sf::Font f, std::string file)
{
	path = file;
	font = f;
}

std::vector<sf::String> AssetManager::getMods()
{
	std::vector<sf::String> mods;
	for (auto p : std::filesystem::directory_iterator(std::filesystem::current_path()))
	{
		auto path = p.path().string();
		if (std::filesystem::exists(path + "/global/settings.trconf"))
		{
			path.erase(0, std::filesystem::current_path().string().length() + 1);
			mods.push_back(path);
		}
	}
	return mods;
}

void AssetManager::init()
{
	auto mods = getMods();
	if (!mods.size()) { exit(0); }
	else if (mods.size() == 1)
	{
		std::vector<char*> args; args.push_back((char*)"game"); args.push_back((char*)sf::String("assets=" + mods[0]).toAnsiString().c_str());
		Window::init(0, args.data());
	}
	else
	{
		std::vector<char*> args; args.push_back((char*)"game"); args.push_back((char*)sf::String("assets=" + mods[0]).toAnsiString().c_str());
		Window::init(0, args.data());
		if (path[path.getSize() - 1] != '/') path += "/";
		bool chosen = false;
		sf::Font font;
		font.loadFromFile(path + "global/font.ttf");
		std::vector<sf::String> names;
		for (int i = 0; i < mods.size(); i++)
		{
			pugi::xml_document file;
			file.load_file(sf::String(mods[i] + "/global/settings.trconf").toWideString().c_str());
			names.push_back(file.child(L"settings").child(L"modification").attribute(L"str").as_string());
		}
		while (!chosen)
		{
			Window::update();
			if (!Window::isOpen()) { exit(0); }
			sf::Text txt("List of mods:", font);
			txt.setOrigin(txt.getGlobalBounds().width / 2, 0);
			txt.setPosition(Window::getSize().x / 2, 0);

			Window::clear();
			Window::draw(txt);
			for (int i = 0; i < mods.size(); i++)
			{
				txt.setString(names[i]);
				txt.setPosition(Window::getSize().x / 2, (i + 1) * 40);
				txt.setOrigin(txt.getGlobalBounds().width / 2, 0);
				if (txt.getGlobalBounds().contains(Input::getMousePos()))
				{
					txt.setFillColor(sf::Color::Red);
					if (Input::isMBJustPressed(sf::Mouse::Left))
					{
						path = mods[i];
						chosen = true;
					}
				}
				else txt.setFillColor(sf::Color::White);
				Window::draw(txt);
			}
			Window::display();
		}
	}
	if (path[path.getSize() - 1] != '/') path += "/";
	sf::Thread thread(&load);
	thread.launch();
	sf::Font f; f.loadFromFile(path + "global/font.ttf");
	sf::Text t("", f, 32);
	float clr = 0;
	auto files = iterateDir(path);
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
	auto files = iterateDir(path);
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
		else if (tr::strContains(file, ".ogg") || tr::strContains(file, ".wav") || tr::strContains(file, ".mp3"))
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

void AssetManager::set(std::string file, sf::Image img)
{
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == file)
		{
			textures[i].tex.loadFromImage(img);
			return;
		}
	}
	Texture t;
	t.path = file;
	t.tex.loadFromImage(img);
	textures.push_back(t);
}

void AssetManager::set(std::string file, sf::Texture img)
{
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == file)
		{
			textures[i].tex = img;
			return;
		}
	}
	textures.push_back(Texture(img, file));
}

void AssetManager::set(std::string file, sf::String text)
{
	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i].path == file)
		{
			texts[i].text = text;
			return;
		}
	}
	texts.push_back(Text(text, file));
}

void AssetManager::set(std::string file, sf::Font font)
{
	for (int i = 0; i < fonts.size(); i++)
	{
		if (fonts[i].path == file) { fonts[i].font = font; return; }
	}
	fonts.push_back(Font(font, file));
}

void AssetManager::set(std::string file, sf::SoundBuffer sb)
{
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].path == file) { sounds[i].music = sb; return; }
	}
	sounds.push_back(Sound(sb, file));
}

sf::Texture *AssetManager::getTexture(std::string file)
{
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == path + file)
		{
			return &textures[i].tex;
		}
	}
	textures.push_back(Texture(path + file));
	return &textures[textures.size() - 1].tex;
}

sf::String AssetManager::getText(std::string file)
{
	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i].path == path + file)
		{
			return texts[i].text;
		}
	}
	texts.push_back(Text(path + file));
	return texts[texts.size() - 1].text;
}

sf::Font *AssetManager::getFont(std::string file)
{
	for (int i = 0; i < fonts.size(); i++)
	{
		if (fonts[i].path == path + file)
		{
			return &fonts[i].font;
		}
	}
	fonts.push_back(Font(path + file));
	return &fonts[fonts.size() - 1].font;
}

sf::SoundBuffer *AssetManager::getSound(std::string file)
{
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].path == path + file)
		{
			return &sounds[i].music;
		}
	}
	sounds.push_back(Sound(path + file));
	return &sounds[sounds.size() - 1].music;
}

std::vector<std::string> AssetManager::getTextures(std::string part)
{
	std::vector<std::string> paths;
	for (int i = 0; i < textures.size(); i++)
	{
		if (tr::strContains(textures[i].path, part))
		{
			auto p = textures[i].path; p.erase(0, p.find("/") + 1);
			paths.push_back(p);
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
			auto p = texts[i].path; p.erase(0, p.find("/") + 1);
			paths.push_back(p);
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
			auto p = fonts[i].path; p.erase(0, p.find("/") + 1);
			paths.push_back(p);
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
			auto p = sounds[i].path; p.erase(0, p.find("/") + 1);
			paths.push_back(p);
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
	if (!music.loadFromFile(path)) throw tr::AssetException(file);
}

AssetManager::Sound::Sound(sf::SoundBuffer sb, std::string file)
{
	path = file;
	music = sb;
}

void AssetManager::reloadTexture(std::string file)
{
	auto tex = Texture(path + file);
	for (int i = 0; i < textures.size(); i++)
	{
		if (textures[i].path == path + file)
		{
			textures[i] = tex;
			return;
		}
	}
	textures.push_back(tex);
}

void AssetManager::reloadText(std::string file)
{
	auto text = Text(path + file);
	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i].path == path + file)
		{
			texts[i] = text;
			return;
		}
	}
	texts.push_back(text);
}

void AssetManager::reloadFont(std::string file)
{
	auto font = Font(path + file);
	for (int i = 0; i < fonts.size(); i++)
	{
		if (fonts[i].path == path + file)
		{
			fonts[i] = font;
			return;
		}
	}
	fonts.push_back(font);
}

void AssetManager::reloadSound(std::string file)
{
	auto sound = Sound(path + file);
	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].path == path + file)
		{
			sounds[i] = sound;
			return;
		}
	}
	sounds.push_back(sound);
}