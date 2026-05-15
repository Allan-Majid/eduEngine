#pragma once

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

struct SphereColliderComponent;
struct AABBColliderComponent;
struct TransformComponent;

class CollisionSystem
{
public:
	void Update(entt::registry& registry);

private:
	bool TestSphereSphere(const glm::vec3& centerA, float radiusA, const glm::vec3& centerB, float radiusB);
	bool TestAABBAABB(const glm::vec3& centerA, const glm::vec3& halfExtentsA, const glm::vec3& centerB, const glm::vec3& halfExtentsB);
	bool TestSphereGroundPlane(const glm::vec3& center, float radius);
	bool TestAABBGroundPlane(const glm::vec3& center, const glm::vec3& halfExtents);
};