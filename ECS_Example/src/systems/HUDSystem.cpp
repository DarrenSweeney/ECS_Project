#include "systems\HUDSystem.h"

HUDSystem::HUDSystem(sf::RenderWindow & window)
	: m_window(window), m_currentNode(0), m_currentLapTime(0.0)
{
	m_HUDFont.loadFromFile(".\\resources\\fonts\\akashi.ttf");
	m_HUDText.setFont(m_HUDFont);
	m_HUDText.setCharacterSize(30);
	m_HUDText.setColor(sf::Color::Black);
}

void HUDSystem::update(entityx::EntityManager& entities,
	entityx::EventManager& events,
	double dt)
{
	std::stringstream ss;
	ss << "Next waypoint number: " << m_currentNode;
	m_HUDText.setString(ss.str());
	m_HUDText.setPosition(sf::Vector2f(40.0f, 40.0f));
	m_window.draw(m_HUDText);

	ss.str("");
	ss << "Lap Time: " << std::setprecision(2) << m_currentLapTime;
	m_HUDText.setString(ss.str());
	m_HUDText.setPosition(sf::Vector2f(40.0f, 100.0f));
	m_window.draw(m_HUDText);
}

void HUDSystem::configure(entityx::EventManager& events)
{
	events.subscribe<EvNode>(*this);
}

void HUDSystem::receive(const EvNode& e)
{
	m_currentNode = e.m_currentNode;
	m_currentLapTime = e.m_currentLapTime;
}