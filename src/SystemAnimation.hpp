#pragma once
#include <entt/entt.hpp>
#include <memory>

namespace eeng
{
	class RenderableMesh;
}

struct AnimationComponent;

class AnimationSystem
{
public:
	void Update(entt::registry& registry, float deltaTime);

private:
	void UpdateSpeedControl(entt::registry& registry, entt::entity entity, AnimationComponent& animationComponent);
	void ApplyLayeredAnimation(std::shared_ptr<eeng::RenderableMesh> mesh, AnimationComponent& animationComponent);
	void ApplyFullBodyBlend(std::shared_ptr<eeng::RenderableMesh> mesh, AnimationComponent& animationComponent);
};
