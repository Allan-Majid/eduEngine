#pragma once

#include <entt/entt.hpp>
#include "InputManager.hpp"

class PlayerControllerSystem
{
public:
	void Update(entt::registry& registry, InputManagerPtr input);
};