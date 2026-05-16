#pragma once

#include <entt/entt.hpp>
#include "InputManager.hpp"

class PlayerControllerSystem
{
public:
	void Update(entt::registry& registry, float deltaTime, float cameraYaw, std::shared_ptr<eeng::InputManager> inputManager);
};