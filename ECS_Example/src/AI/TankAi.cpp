#include "ai/TankAi.h"


TankAi::TankAi(std::vector<sf::CircleShape> const & obstacles, std::vector<sf::CircleShape> const & waypoints,
				entityx::Entity::Id id)
  : m_aiBehaviour(AiBehaviour::PATH_FOLLOWING)
  , m_steering(0,0)
  , m_obstacles(obstacles),
	m_wayPoints(waypoints),
	m_currentLapTime(0.0f)
{
}

void TankAi::update(entityx::Entity::Id playerId,
	entityx::Entity::Id aiId,
	entityx::EntityManager& entities,
	entityx::EventManager& events,
	double dt)
{
	entityx::Entity aiTank = entities.get(aiId);
	Motion::Handle motion = aiTank.component<Motion>();
	Position::Handle position = aiTank.component<Position>();

	events.emit<EvNode>(m_currentNode, m_currentLapTime);

	m_currentLapTime += dt / 10000.0f;

	sf::Vector2f vectorToPlayer = seek(playerId,
		aiId,
		entities);

	sf::Vector2f vectorToNode = pathFollowing(aiId, entities);

	switch (m_aiBehaviour)
	{
	case AiBehaviour::PATH_FOLLOWING:
		m_steering += thor::unitVector(vectorToNode);
		m_steering += collisionAvoidance(aiId, entities);
		m_steering = Math::truncate(m_steering, MAX_FORCE);
		m_velocity = Math::truncate(m_velocity + m_steering, MAX_SPEED);

		break;

	case AiBehaviour::SEEK_PLAYER:
		m_steering += thor::unitVector(vectorToPlayer);
		m_steering += collisionAvoidance(aiId, entities);
		m_steering = Math::truncate(m_steering, MAX_FORCE);
		m_velocity = Math::truncate(m_velocity + m_steering, MAX_SPEED);

		break;
	case AiBehaviour::STOP:
		motion->m_speed = 0;
		break;
	default:
		break;
	}

	// Now we need to convert our velocity vector into a rotation angle between 0 and 359 degrees.
	// The m_velocity vector works like this: vector(1,0) is 0 degrees, while vector(0, 1) is 90 degrees.
	// So for example, 223 degrees would be a clockwise offset from 0 degrees (i.e. along x axis).
	// Note: we add 180 degrees below to convert the final angle into a range 0 to 359 instead of -PI to +PI
	auto dest = atan2(-1 * m_velocity.y, -1 * m_velocity.x) / thor::Pi * 180 + 180; 

	auto currentRotation = position->m_rotation;	

	// Find the shortest way to rotate towards the player (clockwise or anti-clockwise)
	if (std::round(currentRotation - dest) == 0.0)
	{
		m_steering.x = 0;
		m_steering.y = 0;
	}
	else if ((static_cast<int>(std::round(dest - currentRotation + 360))) % 360 < 180)
	{
		// rotate clockwise
		position->m_rotation = static_cast<int>((position->m_rotation) + 1) % 360;
	}
	else
	{
		// rotate anti-clockwise
		position->m_rotation -= 1;  
	}

	motion->m_speed = thor::length(m_velocity);
}

sf::Vector2f TankAi::seek(entityx::Entity::Id playerId,
						  entityx::Entity::Id aiId,
	                      entityx::EntityManager& entities) const
{
	entityx::Entity playerTank = entities.get(playerId);
	Position::Handle playerPos = playerTank.component<Position>();
	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();

	sf::Vector2f playerPosition = playerPos->m_position;
	sf::Vector2f tankPos = aiPos->m_position;
	sf::Vector2f vectorToPlayer = playerPos->m_position - aiPos->m_position;

	return vectorToPlayer;
}

sf::Vector2f TankAi::seekNode(entityx::Entity::Id playerId,
	entityx::Entity::Id aiId,
	entityx::EntityManager& entities) const
{
	return sf::Vector2f();
}

sf::Vector2f TankAi::collisionAvoidance(entityx::Entity::Id aiId, 
									    entityx::EntityManager& entities)
{
	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();	

	auto headingRadians = thor::toRadian(aiPos->m_rotation);	
	sf::Vector2f headingVector(std::cos(headingRadians) * MAX_SEE_AHEAD, std::sin(headingRadians) * MAX_SEE_AHEAD);
	m_ahead = aiPos->m_position + headingVector;	
	m_halfAhead = aiPos->m_position + (headingVector * 0.5f);
	const sf::CircleShape mostThreatening = findMostThreateningObstacle(aiId, entities);
	sf::Vector2f avoidance(0, 0);
	if (mostThreatening.getRadius() != 0.0)
	{
		auto threatPos = mostThreatening.getPosition();
		auto mypos = aiPos->m_position;
		avoidance.x = m_ahead.x - mostThreatening.getPosition().x;
		avoidance.y = m_ahead.y - mostThreatening.getPosition().y;
		avoidance = thor::unitVector(avoidance);
		avoidance *= MAX_AVOID_FORCE;
	}
	else
	{
		avoidance *= 0.0f;
	}
    return avoidance;
}

sf::Vector2f TankAi::pathFollowing(entityx::Entity::Id aiId, entityx::EntityManager& entities)
{
	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();
	sf::Vector2f aiTankPos = aiPos->m_position;

	sf::Vector2f target;

	if (m_wayPoints.size() != 0)
	{
		target = m_wayPoints.at(m_currentNode).getPosition();

		if (Math::distance(target, aiTankPos) < m_wayPoints.at(m_currentNode).getRadius())
		{
			m_currentNode++;

			if (m_currentNode >= m_wayPoints.size())
			{
				m_currentNode = 0;
				m_currentLapTime = 0.0;
			}
		}
	}

	sf::Vector2f direction = m_wayPoints.at(m_currentNode).getPosition() - aiTankPos;

	return direction;
}

const sf::CircleShape TankAi::findMostThreateningObstacle(entityx::Entity::Id aiId,
														  entityx::EntityManager& entities)
{
	sf::CircleShape mostThreatening;
	mostThreatening.setRadius(0.0f);

	entityx::Entity aiTank = entities.get(aiId);
	Position::Handle aiPos = aiTank.component<Position>();

	for (unsigned int i = 0; i < m_obstacles.size(); i++)
	{
		sf::CircleShape obstacle = m_obstacles[i];
		bool collision = Math::lineIntersectsCircle(m_ahead, m_halfAhead, obstacle);

		if (collision && (mostThreatening.getRadius() == 0 
			|| Math::distance(aiPos->m_position, obstacle.getPosition()) <  Math::distance(aiPos->m_position, mostThreatening.getPosition())))
		{
			mostThreatening = obstacle;
		}
	}

	return mostThreatening;
}