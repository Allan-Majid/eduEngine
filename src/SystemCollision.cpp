#include "SystemCollision.hpp"
#include "ComponentTransform.hpp"
#include "ComponentSphereCollider.hpp"
#include "ComponentAABBCollider.hpp"

#include <glm/geometric.hpp>
#include <cmath>
#include <limits>
#include <algorithm>

void CollisionSystem::Update(entt::registry& registry, EventQueue& eventQueue)
{
	auto view = registry.view<TransformComponent, SphereColliderComponent, AABBColliderComponent>();

	std::vector<CollisionSphere> broadPhaseSpheres;

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& sphere = view.get<SphereColliderComponent>(entity);

		if (TestSphereGroundPlane(transform.position, sphere.radius))
		{
			transform.position.y = sphere.radius;
		}

		broadPhaseSpheres.push_back({ entity, transform.position, sphere.radius });
	}

	BVHNode* bvhRoot = BuildBVHBottomUp(broadPhaseSpheres);

	std::vector<std::pair<entt::entity, entt::entity>> possiblePairs;
	FindPossibleCollisionPairs(bvhRoot, possiblePairs);

	for (auto pair : possiblePairs)
	{
		entt::entity entityA = pair.first;
		entt::entity entityB = pair.second;

		if (!registry.valid(entityA) || !registry.valid(entityB))
		{
			continue;
		}

		auto& transformA = registry.get<TransformComponent>(entityA);
		auto& transformB = registry.get<TransformComponent>(entityB);
		auto& sphereA = registry.get<SphereColliderComponent>(entityA);
		auto& sphereB = registry.get<SphereColliderComponent>(entityB);
		auto& aabbA = registry.get<AABBColliderComponent>(entityA);
		auto& aabbB = registry.get<AABBColliderComponent>(entityB);

		bool actualCollision = TestAABBAABB(transformA.position, aabbA.halfExtents, transformB.position, aabbB.halfExtents);

		if (!actualCollision)
		{
			continue;
		}

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

	DeleteBVH(bvhRoot);
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

float CollisionSystem::DistanceBetweenSpheres(const CollisionSphere& leftSphere, const CollisionSphere& rightSphere)
{
	float centerDistance = glm::length(rightSphere.center - leftSphere.center);
	float surfaceDistance = centerDistance - (leftSphere.radius + rightSphere.radius);
	return std::max(0.0f, surfaceDistance);
}

BVHNode* CollisionSystem::BuildNodeFromSingleSphere(const CollisionSphere& sphere)
{
	BVHNode* node = new BVHNode();
	node->sphere = sphere;
	node->isLeaf = true;
	return node;
}

BVHNode* CollisionSystem::BuildNodeFromSpheres(BVHNode* leftNode, BVHNode* rightNode)
{
	glm::vec3 minPoint;
	glm::vec3 maxPoint;

	minPoint.x = std::min(leftNode->sphere.center.x - leftNode->sphere.radius, rightNode->sphere.center.x - rightNode->sphere.radius);
	minPoint.y = std::min(leftNode->sphere.center.y - leftNode->sphere.radius, rightNode->sphere.center.y - rightNode->sphere.radius);
	minPoint.z = std::min(leftNode->sphere.center.z - leftNode->sphere.radius, rightNode->sphere.center.z - rightNode->sphere.radius);

	maxPoint.x = std::max(leftNode->sphere.center.x + leftNode->sphere.radius, rightNode->sphere.center.x + rightNode->sphere.radius);
	maxPoint.y = std::max(leftNode->sphere.center.y + leftNode->sphere.radius, rightNode->sphere.center.y + rightNode->sphere.radius);
	maxPoint.z = std::max(leftNode->sphere.center.z + leftNode->sphere.radius, rightNode->sphere.center.z + rightNode->sphere.radius);

	glm::vec3 center = minPoint + (maxPoint - minPoint) * 0.5f;
	float radius = glm::length(maxPoint - minPoint) * 0.5f;

	BVHNode* node = new BVHNode();
	node->sphere = { entt::null, center, radius };
	node->leftChild = leftNode;
	node->rightChild = rightNode;
	node->isLeaf = false;

	return node;
}

BVHNode* CollisionSystem::BuildBVHBottomUp(std::vector<CollisionSphere>& spheres)
{
	if (spheres.empty())
	{
		return nullptr;
	}

	std::vector<BVHNode*> openList;

	for (const CollisionSphere& sphere : spheres)
	{
		openList.push_back(BuildNodeFromSingleSphere(sphere));
	}

	while (openList.size() > 1)
	{
		float closestDistance = std::numeric_limits<float>::max();
		int bestLeftIndex = 0;
		int bestRightIndex = 1;

		for (int i = 0; i < openList.size(); i++)
		{
			for (int j = i + 1; j < openList.size(); j++)
			{
				float distance = DistanceBetweenSpheres(openList[i]->sphere, openList[j]->sphere);

				if (distance < closestDistance)
				{
					closestDistance = distance;
					bestLeftIndex = i;
					bestRightIndex = j;
				}
			}
		}

		BVHNode* leftNode = openList[bestLeftIndex];
		BVHNode* rightNode = openList[bestRightIndex];
		BVHNode* parentNode = BuildNodeFromSpheres(leftNode, rightNode);

		if (bestLeftIndex > bestRightIndex)
		{
			std::swap(bestLeftIndex, bestRightIndex);
		}

		openList.erase(openList.begin() + bestRightIndex);
		openList.erase(openList.begin() + bestLeftIndex);
		openList.push_back(parentNode);
	}

	return openList[0];
}

void CollisionSystem::FindPossibleCollisionPairs(BVHNode* node, std::vector<std::pair<entt::entity, entt::entity>>& pairs)
{
	if (!node || node->isLeaf)
	{
		return;
	}

	if (node->leftChild && node->rightChild)
	{
		CollectLeafPairs(node->leftChild, node->rightChild, pairs);
	}

	FindPossibleCollisionPairs(node->leftChild, pairs);
	FindPossibleCollisionPairs(node->rightChild, pairs);
}

void CollisionSystem::CollectLeafPairs(BVHNode* leftNode, BVHNode* rightNode, std::vector<std::pair<entt::entity, entt::entity>>& pairs)
{
	if (!leftNode || !rightNode)
	{
		return;
	}

	if (!TestSphereSphere(leftNode->sphere.center, leftNode->sphere.radius, rightNode->sphere.center, rightNode->sphere.radius))
	{
		return;
	}

	if (leftNode->isLeaf && rightNode->isLeaf)
	{
		pairs.push_back({ leftNode->sphere.entity, rightNode->sphere.entity });
		return;
	}

	CollectLeafPairs(leftNode->leftChild, rightNode, pairs);
	CollectLeafPairs(leftNode->rightChild, rightNode, pairs);
	CollectLeafPairs(leftNode, rightNode->leftChild, pairs);
	CollectLeafPairs(leftNode, rightNode->rightChild, pairs);
}

void CollisionSystem::DeleteBVH(BVHNode* node)
{
	if (!node)
	{
		return;
	}

	DeleteBVH(node->leftChild);
	DeleteBVH(node->rightChild);

	delete node;
}