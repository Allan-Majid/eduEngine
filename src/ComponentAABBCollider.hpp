#pragma once

#include <glm/vec3.hpp>

struct AABBColliderComponent
{
	glm::vec3 halfExtents = { 0.5f, 0.5f, 0.5f };
	bool isTrigger = false;
};