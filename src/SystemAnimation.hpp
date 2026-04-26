#pragma once
#include <entt/entt.hpp>

class AnimationSystem
{
	public:
		void Update(entt::registry& registry, float deltaTime);
};