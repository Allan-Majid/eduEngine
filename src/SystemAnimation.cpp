#include "SystemAnimation.hpp"
#include "ComponentAnimation.hpp"
#include "ComponentMesh.hpp"
#include "RenderableMesh.hpp"
#include "ComponentLinearVelocity.hpp"
#include <glm/geometric.hpp>
#include <algorithm>

void AnimationSystem::Update(entt::registry& registry, float deltaTime)
{
	auto view = registry.view<AnimationComponent, MeshComponent>();

	for (auto entity : view)
	{
		auto& animationComponent = view.get<AnimationComponent>(entity);
		auto& meshComponent = view.get<MeshComponent>(entity);

		auto mesh = meshComponent.mesh.lock();

		if (!mesh)
		{
			continue;
		}

		if (!animationComponent.animationFinished)
		{
			animationComponent.time += deltaTime;

			if (animationComponent.playOnce && animationComponent.time >= animationComponent.animationDuration)
			{
				animationComponent.time = animationComponent.animationDuration;
				animationComponent.animationFinished = true;
			}
		}

		if (animationComponent.useLayering)
		{
			ApplyLayeredAnimation(mesh, animationComponent);
		}
		else
		{
			if (animationComponent.useSpeedControl)
			{
				UpdateSpeedControl(registry, entity, animationComponent);
			}

			ApplyFullBodyBlend(mesh, animationComponent);
		}
	}
}

void AnimationSystem::UpdateSpeedControl(entt::registry& registry, entt::entity entity, AnimationComponent& animationComponent)
{
	float movementSpeed = 0.0f;

	if (auto* velocityComponent = registry.try_get<LinearVelocityComponent>(entity))
	{
		movementSpeed = glm::length(velocityComponent->velocity);
	}

	float animationWalkSpeed = 10.0f;
	float targetSpeed = std::clamp(movementSpeed / animationWalkSpeed, 0.0f, 1.0f);

	animationComponent.speed = glm::mix(animationComponent.speed, targetSpeed, 0.1f);

	animationComponent.blendFactor = animationComponent.speed;
}

void AnimationSystem::ApplyLayeredAnimation(std::shared_ptr<eeng::RenderableMesh> mesh, AnimationComponent& animationComponent)
{
	if (animationComponent.upperBodyRootNode.empty())
	{
		ApplyFullBodyBlend(mesh, animationComponent);
		return;
	}

	eeng::AnimationBranchDesc upperBodyFilter;
	upperBodyFilter.root_node_name = animationComponent.upperBodyRootNode;

	mesh->animateBlend(animationComponent.baseAnimation, animationComponent.secondaryAnimation, animationComponent.time, animationComponent.time, upperBodyFilter);
}

void AnimationSystem::ApplyFullBodyBlend(std::shared_ptr<eeng::RenderableMesh> mesh, AnimationComponent& animationComponent)
{
	mesh->animateBlend(animationComponent.baseAnimation, animationComponent.secondaryAnimation, animationComponent.time, animationComponent.time, animationComponent.blendFactor);
}