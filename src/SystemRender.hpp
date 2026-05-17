#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <glm/glm.hpp>

#include "ForwardRenderer.hpp"
#include "ShapeRenderer.hpp"
#include "RenderableMesh.hpp"

class RenderSystem
{
public:
	void Render(
		entt::registry& registry,
		eeng::ForwardRendererPtr renderer,
		ShapeRendererPtr shapeRenderer,
		float time,
		bool drawCollisionDebug
	);

private:
	void RenderDebugGizmo(
		ShapeRendererPtr shapeRenderer,
		std::shared_ptr<eeng::RenderableMesh> meshPtr,
		const glm::mat4& worldMatrix
	);
	void RenderCollisionDebug(entt::registry& registry, ShapeRendererPtr shapeRenderer);
};