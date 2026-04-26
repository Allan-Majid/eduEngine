#include "SystemAnimation.hpp"
#include "ComponentAnimation.hpp"
#include "ComponentMesh.hpp"
#include "RenderableMesh.hpp"


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

		}
		else
		{
			mesh->animateBlend(animationComponent.baseAnimation, animationComponent.secondaryAnimation, animationComponent.time, animationComponent.time, animationComponent.blendFactor);
		}
	}
}