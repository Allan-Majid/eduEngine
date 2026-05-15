#include "SystemCollision.hpp"
#include "ComponentTransform.hpp"
#include "ComponentSphereCollider.hpp"
#include "ComponentAABBCollider.hpp"

#include <glm/geometric.hpp>
#include <cmath>

void CollisionSystem::Update(entt::registry& registry, EventQueue& eventQueue)
{
	auto view = registry.view<TransformComponent, SphereColliderComponent, AABBColliderComponent>();

	for (auto entityA : view)
	{
		auto& transformA = view.get<TransformComponent>(entityA);
		auto& sphereA = view.get<SphereColliderComponent>(entityA);
		auto& aabbA = view.get<AABBColliderComponent>(entityA);

		if (TestSphereGroundPlane(transformA.position, sphereA.radius))
		{
			transformA.position.y = sphereA.radius;
		}

		for (auto entityB : view)
		{
			if (entityA == entityB || entityA > entityB)
			{
				continue;
			}

			auto& transformB = view.get<TransformComponent>(entityB);
			auto& sphereB = view.get<SphereColliderComponent>(entityB);
			auto& aabbB = view.get<AABBColliderComponent>(entityB);

			bool possibleCollision = TestSphereSphere(transformA.position, sphereA.radius, transformB.position, sphereB.radius);

			if (!possibleCollision)
			{
				continue;
			}

			bool actualCollision = TestAABBAABB(transformA.position, aabbA.halfExtents, transformB.position, aabbB.halfExtents);

			if (actualCollision)
			{
				bool bothAreTriggers = sphereA.isTrigger && sphereB.isTrigger;
				bool oneIsTrigger = sphereA.isTrigger || sphereB.isTrigger;

				if (bothAreTriggers)
				{
					continue;
				}

				if (oneIsTrigger)
				{
					eventQueue.EnqueueEvent({ GameEventType::TriggerEntered, entityA, entityB, "Trigger entered" });
					continue;
				}

				glm::vec3 direction = transformA.position - transformB.position;

				if (glm::length(direction) == 0.0f)
				{
					direction = { 1.0f, 0.0f, 0.0f };
				}
				else
				{
					direction = glm::normalize(direction);
				}

				float overlap = (sphereA.radius + sphereB.radius) - glm::length(transformA.position - transformB.position);

				if (overlap > 0.0f)
				{
					transformA.position += direction * (overlap * 0.5f);
					transformB.position -= direction * (overlap * 0.5f);
				}

				eventQueue.EnqueueEvent({ GameEventType::CollisionStarted, entityA, entityB, "Collision resolved" });
			}
		}
	}
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