#ifndef HUD_SYSTEM_H
#define HUG_SYSTEM_H

#include <SFML/Graphics.hpp>
#include "entityx/System.h"
#include "entityx/Event.h"
#include "systems/Events.h"
#include <iomanip>	// Set Precision

class HUDSystem 
	: public entityx::System<HUDSystem>, public entityx::Receiver<HUDSystem>
{
public:
	HUDSystem(sf::RenderWindow& window);

	void update(entityx::EntityManager& entities,
		entityx::EventManager& events,
		double dt) override;

	void configure(entityx::EventManager& events);

	void receive(const EvNode& e);

private:
	sf::RenderWindow& m_window;
	sf::Font m_HUDFont;
	sf::Text m_HUDText;
	int m_currentNode;
	double m_currentLapTime;
};

#endif