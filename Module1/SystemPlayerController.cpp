#include "SystemPlayerController.hpp"
#include "ComponentPlayerController.hpp"
#include "ComponentLinearVelocity.hpp"
#include "ComponentTransform.hpp"
#include "glmcommon.hpp"

void PlayerControllerSystem::Update(entt::registry& registry, float deltaTime, float cameraYaw, std::shared_ptr<eeng::InputManager> inputManager)
{
    using Key = eeng::InputManager::Key;

    bool W = inputManager->IsKeyPressed(Key::W);
    bool A = inputManager->IsKeyPressed(Key::A);
    bool S = inputManager->IsKeyPressed(Key::S);
    bool D = inputManager->IsKeyPressed(Key::D);

    auto view = registry.view<PlayerControllerComponent, LinearVelocityComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto& playerController = view.get<PlayerControllerComponent>(entity);
        auto& velocity = view.get<LinearVelocityComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        glm::vec3 cameraForward = glm::vec3(glm_aux::R(cameraYaw, glm_aux::vec3_010) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        cameraForward.y = 0.0f;

        if (glm::length(cameraForward) > 0.0f)
        {
            cameraForward = glm::normalize(cameraForward);
        }

        glm::vec3 cameraRight = glm::cross(cameraForward, glm_aux::vec3_010);

        glm::vec3 moveDirection = { 0.0f, 0.0f, 0.0f };

        if (W)
        {
            moveDirection += cameraForward;
        }

        if (S)
        {
            moveDirection -= cameraForward;
        }

        if (D)
        {
            moveDirection += cameraRight;
        }

        if (A)
        {
            moveDirection -= cameraRight;
        }

        if (glm::length(moveDirection) > 0.0f)
        {
            moveDirection = glm::normalize(moveDirection);
            velocity.velocity = moveDirection * playerController.movementSpeed;
            transform.rotation.y = glm::degrees(std::atan2(moveDirection.x, moveDirection.z));
        }
        else
        {
            velocity.velocity = { 0.0f, 0.0f, 0.0f };
        }
    }
}