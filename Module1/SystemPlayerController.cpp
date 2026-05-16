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

		moveDirection = glm::normalize(moveDirection);
		bool hasMovementInput = glm::length(moveDirection) > 0.0f;

		if (hasMovementInput)
		{
			moveDirection = glm::normalize(moveDirection);
			playerController.lastMoveDirection = moveDirection;

			playerController.currentSpeed += playerController.acceleration * deltaTime;

			if (playerController.currentSpeed > playerController.movementSpeed)
			{
				playerController.currentSpeed = playerController.movementSpeed;
			}

			velocity.velocity = moveDirection * playerController.currentSpeed;

			float targetYaw = glm::degrees(std::atan2(moveDirection.x, moveDirection.z));
			float rotationSpeed = 10.0f;
			float currentYaw = transform.rotation.y;
			float deltaYaw = targetYaw - currentYaw;

			while (deltaYaw > 180.0f)
			{
				deltaYaw -= 360.0f;
			}

			while (deltaYaw < -180.0f)
			{
				deltaYaw += 360.0f;
			}

			transform.rotation.y += deltaYaw * rotationSpeed * deltaTime;
		}
		else
		{
			playerController.currentSpeed = 0.0f;
			velocity.velocity = { 0.0f, 0.0f, 0.0f };
		}

	}
}