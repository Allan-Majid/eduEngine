#pragma once

#include <glm/vec3.hpp>

struct AABBColliderComponent
{
	glm::vec3 halfExtents = { 0.5f, 0.5f, 0.5f };
	glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
	bool isTrigger = false;
	bool isStatic = false;
};