#pragma once

struct SphereColliderComponent
{
	float radius = 1.0f;
	glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
	bool isTrigger = false;
	bool isStatic = false;
};