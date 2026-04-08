#pragma once

#include <vector>
#include <glm/vec3.hpp>

struct NPCControllerComponent
{
	std::vector<glm::vec3> waypoints;
	int currentWaypointIndex = 0;
	float movementSpeed = 3.0f;
	float arriveDistance = 0.5f; // Distance at which the NPC is considered to have arrived at the waypoint
};
