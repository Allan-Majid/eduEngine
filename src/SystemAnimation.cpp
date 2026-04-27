#include "SystemAnimation.hpp"
#include "ComponentAnimation.hpp"
#include "ComponentMesh.hpp"
#include "RenderableMesh.hpp"
#include "ComponentLinearVelocity.hpp"
#include "ComponentNPCController.hpp"
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

		animationComponent.time += deltaTime;

		if (animationComponent.useLayering)
		{
			if (animationComponent.upperBodyRootNode.empty())
			{
				mesh->animateBlend(animationComponent.baseAnimation, animationComponent.secondaryAnimation, animationComponent.time, animationComponent.time, animationComponent.blendFactor);
				continue;
			}

			eeng::AnimationBranchDesc upperBodyFilter;
			upperBodyFilter.root_node_name = animationComponent.upperBodyRootNode;

			mesh->animateBlend(animationComponent.baseAnimation, animationComponent.secondaryAnimation, animationComponent.time, animationComponent.time, upperBodyFilter);
		}
		else
		{
			if (animationComponent.useSpeedControl)
			{
				float movementSpeed = 0.0f;

				if (auto* velocityComponent = registry.try_get<LinearVelocityComponent>(entity))
				{
					movementSpeed = glm::length(velocityComponent->velocity);
				}

				float animationWalkSpeed = 4.0f;
				float targetSpeed = std::clamp(movementSpeed / animationWalkSpeed, 0.0f, 1.0f);

				animationComponent.speed = glm::mix(animationComponent.speed, targetSpeed, 0.1f);

				animationComponent.baseAnimation = 1;      
				animationComponent.secondaryAnimation = 2; 
				animationComponent.blendFactor = animationComponent.speed;
			}

			mesh->animateBlend(animationComponent.baseAnimation, animationComponent.secondaryAnimation, animationComponent.time, animationComponent.time, animationComponent.blendFactor);
		}
	}
}