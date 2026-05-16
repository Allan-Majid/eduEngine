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

		Plane groundPlane;
		groundPlane.normal = { 0.0f, 1.0f, 0.0f };
		groundPlane.distanceToOrigin = 0.0f;

		Sphere entitySphere;
		entitySphere.center = transform.position + sphere.offset;
		entitySphere.radius = sphere.radius;

		if (SpherePlaneIntersection(entitySphere, groundPlane))
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

		AABB entityAABB_A;
		entityAABB_A.center = transformA.position;
		entityAABB_A.halfWidths = aabbA.halfExtents;

		AABB entityAABB_B;
		entityAABB_B.center = transformB.position;
		entityAABB_B.halfWidths = aabbB.halfExtents;

		bool actualCollision = TestAABBAABB(entityAABB_A, entityAABB_B);

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

		Sphere entitySphereA;
		entitySphereA.center = transformA.position;
		entitySphereA.radius = sphereA.radius;

		Sphere entitySphereB;
		entitySphereB.center = transformB.position;
		entitySphereB.radius = sphereB.radius;

		SimpleCollisionStruct* collisionData = SphereSphere(entitySphereA, entitySphereB, entityA, entityB);

		if (collisionData)
		{
			bool staticA = sphereA.isStatic;
			bool staticB = sphereB.isStatic;

			if (!staticA && !staticB)
			{
				SeparateSpheres(transformA, transformB, *collisionData);
			}
			else if (!staticA && staticB)
			{
				glm::vec3 separationDirection = GetAABBSeparationDirection(entityAABB_A, entityAABB_B);
				float penetrationDepth = GetAABBPenetrationDepth(entityAABB_A, entityAABB_B);
				transformA.position += separationDirection * penetrationDepth;
			}
			else if (staticA && !staticB)
			{
				glm::vec3 separationDirection = GetAABBSeparationDirection(entityAABB_B, entityAABB_A);
				float penetrationDepth = GetAABBPenetrationDepth(entityAABB_B, entityAABB_A);
				transformB.position += separationDirection * penetrationDepth;
			}

			delete collisionData;
		}

		eventQueue.EnqueueEvent({ GameEventType::CollisionStarted, entityA, entityB, "Collision resolved" });
	}

	DeleteBVH(bvhRoot);
}

bool CollisionSystem::TestSphereSphere(const Sphere& a, const Sphere& b)
{
	glm::vec3 centerToCenter = a.center - b.center;
	float distance = glm::dot(centerToCenter, centerToCenter);
	float radiusSum = a.radius + b.radius;

	return distance <= radiusSum * radiusSum;
}

bool CollisionSystem::TestAABBAABB(const AABB& a, const AABB& b)
{
	float centerDiff = std::abs(a.center.x - b.center.x);
	float compoundedWidth = a.halfWidths.x + b.halfWidths.x;

	if (centerDiff > compoundedWidth)
	{
		return false;
	}

	centerDiff = std::abs(a.center.y - b.center.y);
	compoundedWidth = a.halfWidths.y + b.halfWidths.y;

	if (centerDiff > compoundedWidth)
	{
		return false;
	}

	centerDiff = std::abs(a.center.z - b.center.z);
	compoundedWidth = a.halfWidths.z + b.halfWidths.z;

	if (centerDiff > compoundedWidth)
	{
		return false;
	}

	return true;
}

bool CollisionSystem::SpherePlaneIntersection(const Sphere& sphere, const Plane& plane)
{
	float distance = glm::dot(sphere.center, plane.normal) - plane.distanceToOrigin;

	return distance <= sphere.radius;
}

bool CollisionSystem::AABBPlaneIntersection(const AABB& aabb, const Plane& plane)
{
	float r = aabb.halfWidths.x * std::abs(plane.normal.x) +
		aabb.halfWidths.y * std::abs(plane.normal.y) +
		aabb.halfWidths.z * std::abs(plane.normal.z);

	float s = glm::dot(plane.normal, aabb.center) - plane.distanceToOrigin;

	return s <= r;
}

SimpleCollisionStruct* CollisionSystem::SphereSphere(const Sphere& a, const Sphere& b, entt::entity entityA, entt::entity entityB)
{
	glm::vec3 centerToCenter = a.center - b.center;
	float distance = glm::dot(centerToCenter, centerToCenter);
	float radiusSum = a.radius + b.radius;

	if (distance > radiusSum * radiusSum)
	{
		return nullptr;
	}

	SimpleCollisionStruct* collisionData = new SimpleCollisionStruct();

	if (distance == 0.0f)
	{
		collisionData->contactNormal = { 1.0f, 0.0f, 0.0f };
	}
	else
	{
		collisionData->contactNormal = glm::normalize(centerToCenter);
	}

	collisionData->thisEntity = entityA;
	collisionData->otherEntity = entityB;
	collisionData->contactPoint = a.center - collisionData->contactNormal * a.radius;
	collisionData->penetrationDepth = radiusSum - std::sqrt(distance);

	return collisionData;
}

void CollisionSystem::SeparateSpheres(TransformComponent& transformA, TransformComponent& transformB, const SimpleCollisionStruct& collisionData)
{
	transformA.position += collisionData.contactNormal * (collisionData.penetrationDepth * 0.5f);
	transformB.position -= collisionData.contactNormal * (collisionData.penetrationDepth * 0.5f);
}

glm::vec3 CollisionSystem::GetAABBSeparationDirection(const AABB& a, const AABB& b)
{
	float overlapX = (a.halfWidths.x + b.halfWidths.x) - std::abs(a.center.x - b.center.x);
	float overlapY = (a.halfWidths.y + b.halfWidths.y) - std::abs(a.center.y - b.center.y);
	float overlapZ = (a.halfWidths.z + b.halfWidths.z) - std::abs(a.center.z - b.center.z);

	if (overlapX <= overlapY && overlapX <= overlapZ)
	{
		return { a.center.x < b.center.x ? -1.0f : 1.0f, 0.0f, 0.0f };
	}

	if (overlapY <= overlapX && overlapY <= overlapZ)
	{
		return { 0.0f, a.center.y < b.center.y ? -1.0f : 1.0f, 0.0f };
	}

	return { 0.0f, 0.0f, a.center.z < b.center.z ? -1.0f : 1.0f };
}

float CollisionSystem::GetAABBPenetrationDepth(const AABB& a, const AABB& b)
{
	float overlapX = (a.halfWidths.x + b.halfWidths.x) - std::abs(a.center.x - b.center.x);
	float overlapY = (a.halfWidths.y + b.halfWidths.y) - std::abs(a.center.y - b.center.y);
	float overlapZ = (a.halfWidths.z + b.halfWidths.z) - std::abs(a.center.z - b.center.z);

	return std::min(overlapX, std::min(overlapY, overlapZ));
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

	Sphere leftSphere;
	leftSphere.center = leftNode->sphere.center;
	leftSphere.radius = leftNode->sphere.radius;

	Sphere rightSphere;
	rightSphere.center = rightNode->sphere.center;
	rightSphere.radius = rightNode->sphere.radius;

	if (!TestSphereSphere(leftSphere, rightSphere))
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