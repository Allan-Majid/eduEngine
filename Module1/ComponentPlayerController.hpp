#pragma once

#include <glm/vec3.hpp>

struct PlayerControllerComponent
{
	float movementSpeed = 10.0f;
	float currentSpeed = 0.0f;
	float acceleration = 8.0f;
	float deceleration = 30.0f;
	glm::vec3 lastMoveDirection = { 0.0f, 0.0f, 1.0f };
};
