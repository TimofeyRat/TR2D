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
	Weapon weapon;
	Bauble bauble;
	Entity();
	Entity(std::string filename);
	void reset();
	void loadFromFile(std::string filename);
	void update();
	void draw(sf::RenderTarget *target);
	void setPosition(sf::Vector2f pos);
	sf::Vector2f getPosition();
	sf::FloatRect getHitbox();
	void addEffect(Inventory::Effect effect);
	bool isAlive();
	Skeleton *getSkeleton();
};

#endif