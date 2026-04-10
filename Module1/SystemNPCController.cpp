#include "SystemNPCController.hpp"
#include "ComponentNPCController.hpp"
#include "ComponentTransform.hpp"
#include "ComponentLinearVelocity.hpp"
#include <glm/geometric.hpp>

void NPCControllerSystem::Update(entt::registry& registry)
{
    auto view = registry.view<NPCControllerComponent, TransformComponent, LinearVelocityComponent>();

    for (auto entity : view)
    {
        auto& npc = view.get<NPCControllerComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        auto& velocity = view.get<LinearVelocityComponent>(entity);

        if (npc.waypoints.empty())
        {
            velocity.velocity = { 0.0f, 0.0f, 0.0f };
            continue;
        }

        glm::vec3 target = npc.waypoints[npc.currentWaypointIndex];
        glm::vec3 toTarget = target - transform.position;

        toTarget.y = 0.0f;

        float distance = glm::length(toTarget);

        if (distance <= npc.arriveDistance)
        {
            npc.currentWaypointIndex = (npc.currentWaypointIndex + 1) % npc.waypoints.size();
            velocity.velocity = { 0.0f, 0.0f, 0.0f };
            continue;
        }

        glm::vec3 direction = glm::normalize(toTarget);
        velocity.velocity = direction * npc.movementSpeed;
        transform.rotation.y = std::atan2(direction.x, direction.z);
    }
}