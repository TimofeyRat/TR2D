#ifndef trEntity
#define trEntity

#include "hAnimation.hpp"
#include "hProgrammable.hpp"
#include "hRigidbody.hpp"
#include "hInventory.hpp"
#include "hWearable.hpp"

class Entity : public Programmable
{
private:
	Skeleton s;
	Rigidbody rb;
	void updateAnim();
	std::vector<Inventory::Effect> effects;
	void applyEffects();
	void updateRB(float scale);
	void updateAttack();
public:
	sf::String name;
	Weapon weapon;
	Bauble bauble;
	Entity();
	Entity(sf::String file);
	Entity(pugi::xml_node node);
	void reset();
	void loadFromNode(pugi::xml_node node);
	void loadFromFile(sf::String file);
	void update();
	void draw(sf::RenderTarget *target, const sf::RenderStates &states = sf::RenderStates::Default);
	void setPosition(sf::Vector2f pos);
	sf::Vector2f getPosition();
	sf::FloatRect getHitbox();
	void addEffect(Inventory::Effect effect);
	bool isAlive();
	Skeleton *getSkeleton();
	Rigidbody *getRigidbody();
};

#endif