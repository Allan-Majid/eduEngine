#include "SystemCollision.hpp"
#include "ComponentTransform.hpp"
#include "ComponentSphereCollider.hpp"
#include "ComponentAABBCollider.hpp"

#include <glm/geometric.hpp>
#include <cmath>

void CollisionSystem::Update(entt::registry& registry)
{
	//TODO: Implement entity iteration
}

bool CollisionSystem::TestSphereSphere(const glm::vec3& centerA, float radiusA, const glm::vec3& centerB, float radiusB)
{
	glm::vec3 centerToCenter = centerA - centerB;
	float distanceSquared = glm::dot(centerToCenter, centerToCenter);
	float radiusSum = radiusA + radiusB;

	return distanceSquared <= radiusSum * radiusSum;
}

bool CollisionSystem::TestAABBAABB(const glm::vec3& centerA, const glm::vec3& halfExtentsA, const glm::vec3& centerB, const glm::vec3& halfExtentsB)
{
	bool overlapX = std::abs(centerA.x - centerB.x) <= (halfExtentsA.x + halfExtentsB.x);
	bool overlapY = std::abs(centerA.y - centerB.y) <= (halfExtentsA.y + halfExtentsB.y);
	bool overlapZ = std::abs(centerA.z - centerB.z) <= (halfExtentsA.z + halfExtentsB.z);

	return overlapX && overlapY && overlapZ;
}

bool CollisionSystem::TestSphereGroundPlane(const glm::vec3& center, float radius)
{
	return center.y - radius <= 0.0f;
}

bool CollisionSystem::TestAABBGroundPlane(const glm::vec3& center, const glm::vec3& halfExtents)
{
	return center.y - halfExtents.y <= 0.0f;
}