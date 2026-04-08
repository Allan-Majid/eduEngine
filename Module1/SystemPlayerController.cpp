#include "SystemPlayerController.hpp"
#include "ComponentPlayerController.hpp"
#include "ComponentLinearVelocity.hpp"
#include "glmcommon.hpp"

void PlayerControllerSystem::Update(entt::registry& registry, InputManagerPtr input)
{
    using Key = eeng::InputManager::Key;

    bool W = input->IsKeyPressed(Key::W);
    bool A = input->IsKeyPressed(Key::A);
    bool S = input->IsKeyPressed(Key::S);
    bool D = input->IsKeyPressed(Key::D);

    auto view = registry.view<PlayerControllerComponent, LinearVelocityComponent>();

    for (auto entity : view)
    {
        auto& playerController = view.get<PlayerControllerComponent>(entity);
        auto& velocity = view.get<LinearVelocityComponent>(entity);

        glm::vec3 moveDirection = { 0.0f, 0.0f, 0.0f };

        if (W) moveDirection.z -= 1.0f;
        if (S) moveDirection.z += 1.0f;
        if (A) moveDirection.x -= 1.0f;
        if (D) moveDirection.x += 1.0f;

        if (glm::length(moveDirection) > 0.0f)
        {
            moveDirection = glm::normalize(moveDirection);
        }

        velocity.velocity = moveDirection * playerController.movementSpeed;
    }
}