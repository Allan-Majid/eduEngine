#pragma once

#include "SystemMovement.hpp"
#include "ComponentTransform.hpp"
#include "ComponentLinearVelocity.hpp"

void MovementSystem::Update(entt::registry& registry, float deltaTime)
{
	auto view = registry.view<TransformComponent, LinearVelocityComponent>();
	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& velocity = view.get<LinearVelocityComponent>(entity);
		transform.position += velocity.velocity * deltaTime;
	}
}