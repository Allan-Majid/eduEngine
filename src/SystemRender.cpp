#include "SystemRender.hpp"
#include "ComponentTransform.hpp"
#include "ComponentMesh.hpp"
#include "ComponentAnimation.hpp"
#include "glmcommon.hpp"

#include <glm/gtc/matrix_transform.hpp>

void RenderSystem::Render(entt::registry& registry, eeng::ForwardRendererPtr renderer, ShapeRendererPtr shapeRenderer, float time)
{
	auto view = registry.view<TransformComponent, MeshComponent>();

	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);
		auto& meshComp = view.get<MeshComponent>(entity);
		auto meshPtr = meshComp.mesh.lock();

		if (!meshPtr)
			continue;

		glm::mat4 worldMatrix = glm_aux::TRS(
			transform.position,
			glm::radians(transform.rotation.y), { 0, 1, 0 },
			transform.scale
		);

		renderer->renderMesh(meshPtr, worldMatrix);

		if (auto* animationComponent = registry.try_get<AnimationComponent>(entity);
			animationComponent && animationComponent->drawSkeleton)
		{
			RenderDebugGizmo(shapeRenderer, meshPtr, worldMatrix);
		}
	}
}

void RenderSystem::RenderDebugGizmo(ShapeRendererPtr shapeRenderer, std::shared_ptr<eeng::RenderableMesh> meshPtr, const glm::mat4& worldMatrix)
{
	float axisLen = 10.0f; 

	for (int i = 0; i < meshPtr->boneMatrices.size(); ++i)
	{
		auto IBinverse = glm::inverse(meshPtr->m_bones[i].inversebind_tfm);

		glm::mat4 global =
			worldMatrix *
			meshPtr->boneMatrices[i] *
			IBinverse;

		glm::vec3 pos = glm::vec3(global[3]);

		glm::vec3 right = glm::vec3(global[0]);
		glm::vec3 up = glm::vec3(global[1]);
		glm::vec3 fwd = glm::vec3(global[2]);

		shapeRenderer->push_states(ShapeRendering::Color4u::Red);
		shapeRenderer->push_line(pos, pos + axisLen * right);

		shapeRenderer->push_states(ShapeRendering::Color4u::Green);
		shapeRenderer->push_line(pos, pos + axisLen * up);

		shapeRenderer->push_states(ShapeRendering::Color4u::Blue);
		shapeRenderer->push_line(pos, pos + axisLen * fwd);

		shapeRenderer->pop_states<ShapeRendering::Color4u>();
		shapeRenderer->pop_states<ShapeRendering::Color4u>();
		shapeRenderer->pop_states<ShapeRendering::Color4u>();
	}
}