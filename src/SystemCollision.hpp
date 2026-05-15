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

class CollisionSystem
{
public:
	void Update(entt::registry& registry, EventQueue& eventQueue);

private:
	bool TestSphereSphere(const glm::vec3& centerA, float radiusA, const glm::vec3& centerB, float radiusB);
	bool TestAABBAABB(const glm::vec3& centerA, const glm::vec3& halfExtentsA, const glm::vec3& centerB, const glm::vec3& halfExtentsB);
	bool TestSphereGroundPlane(const glm::vec3& center, float radius);
	bool TestAABBGroundPlane(const glm::vec3& center, const glm::vec3& halfExtents);
	BVHNode* BuildBVHBottomUp(std::vector<CollisionSphere>& spheres);
	BVHNode* BuildNodeFromSingleSphere(const CollisionSphere& sphere);
	BVHNode* BuildNodeFromSpheres(BVHNode* leftNode, BVHNode* rightNode);
	void FindPossibleCollisionPairs(BVHNode* node, std::vector<std::pair<entt::entity, entt::entity>>& pairs);
	void CollectLeafPairs(BVHNode* leftNode, BVHNode* rightNode, std::vector<std::pair<entt::entity, entt::entity>>& pairs);
	void DeleteBVH(BVHNode* node);
	float DistanceBetweenSpheres(const CollisionSphere& leftSphere, const CollisionSphere& rightSphere);

};