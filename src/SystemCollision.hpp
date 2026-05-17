#pragma once

#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <utility>
#include "EventQueue.hpp"


struct SphereColliderComponent;
struct AABBColliderComponent;
struct TransformComponent;

struct CollisionSphere
{
	entt::entity entity = entt::null;
	glm::vec3 center = { 0.0f, 0.0f, 0.0f };
	float radius = 1.0f;
};

struct BVHNode
{
	CollisionSphere sphere;
	BVHNode* leftChild = nullptr;
	BVHNode* rightChild = nullptr;
	bool isLeaf = false;
};

struct Sphere
{
	glm::vec3 center = { 0.0f, 0.0f, 0.0f };
	float radius = 1.0f;
};

struct AABB
{
	glm::vec3 center = { 0.0f, 0.0f, 0.0f };
	glm::vec3 halfWidths = { 0.5f, 0.5f, 0.5f };
};

struct Plane
{
	glm::vec3 normal = { 0.0f, 1.0f, 0.0f };
	float distanceToOrigin = 0.0f;
};

struct SimpleCollisionStruct
{
	entt::entity thisEntity = entt::null;
	entt::entity otherEntity = entt::null;
	float penetrationDepth = 0.0f;
	glm::vec3 contactPoint = { 0.0f, 0.0f, 0.0f };
	glm::vec3 contactNormal = { 0.0f, 0.0f, 0.0f };
};

class CollisionSystem
{
public:
	void Update(entt::registry& registry, EventQueue& eventQueue);

private:
	bool TestSphereSphere(const Sphere& a, const Sphere& b);
	bool TestAABBAABB(const AABB& a, const AABB& b);
	bool SpherePlaneIntersection(const Sphere& sphere, const Plane& plane);
	bool AABBPlaneIntersection(const AABB& aabb, const Plane& plane);
	SimpleCollisionStruct* SphereSphere(const Sphere& a, const Sphere& b, entt::entity entityA, entt::entity entityB);
	void ResolveSphereCollision(TransformComponent& transformA, TransformComponent& transformB, const SimpleCollisionStruct& collisionData);
	float GetAABBPenetrationDepth(const AABB& a, const AABB& b);
	glm::vec3 GetAABBSeparationDirection(const AABB& a, const AABB& b);
	std::vector<std::pair<entt::entity, entt::entity>> previousTriggerPairs;
	bool ContainsPair(const std::vector<std::pair<entt::entity, entt::entity>>& pairs, entt::entity triggerEntity, entt::entity colliderEntity);

	// BVH construction and collision pair finding
	BVHNode* BuildBVHBottomUp(std::vector<CollisionSphere>& spheres);
	BVHNode* BuildNodeFromSingleSphere(const CollisionSphere& sphere);
	BVHNode* BuildNodeFromSpheres(BVHNode* leftNode, BVHNode* rightNode);
	void FindPossibleCollisionPairs(BVHNode* node, std::vector<std::pair<entt::entity, entt::entity>>& pairs);
	void CollectLeafPairs(BVHNode* leftNode, BVHNode* rightNode, std::vector<std::pair<entt::entity, entt::entity>>& pairs);
	void DeleteBVH(BVHNode* node);
	float DistanceBetweenSpheres(const CollisionSphere& leftSphere, const CollisionSphere& rightSphere);

};

