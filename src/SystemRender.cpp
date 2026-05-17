#include "SystemRender.hpp"
#include "ComponentTransform.hpp"
#include "ComponentMesh.hpp"
#include "ComponentAnimation.hpp"
#include "glmcommon.hpp"
#include "ComponentSphereCollider.hpp"
#include "ComponentAABBCollider.hpp"

#include <glm/gtc/matrix_transform.hpp>

void RenderSystem::Render(entt::registry& registry, eeng::ForwardRendererPtr renderer, ShapeRendererPtr shapeRenderer, float time, bool drawCollisionDebug)
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

	if (drawCollisionDebug)
	{
		RenderCollisionDebug(registry, shapeRenderer);
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

void RenderSystem::RenderCollisionDebug(entt::registry& registry, ShapeRendererPtr shapeRenderer)
{
	auto colliderView = registry.view<TransformComponent, AABBColliderComponent>();

	shapeRenderer->push_states(ShapeRendering::Color4u{ 0xFF00FFFF });

	for (auto entity : colliderView)
	{
		auto& transform = colliderView.get<TransformComponent>(entity);
		auto& aabb = colliderView.get<AABBColliderComponent>(entity);

		glm::vec3 center = transform.position + aabb.offset;

		glm::vec3 min = center - aabb.halfExtents;
		glm::vec3 max = center + aabb.halfExtents;

		shapeRenderer->push_AABB(min, max);
	}

	shapeRenderer->pop_states<ShapeRendering::Color4u>();

	auto sphereView = registry.view<TransformComponent, SphereColliderComponent>();

	shapeRenderer->push_states(ShapeRendering::Color4u{ 0xFFFF0000 });

	for (auto entity : sphereView)
	{
		auto& transform = sphereView.get<TransformComponent>(entity);
		auto& sphere = sphereView.get<SphereColliderComponent>(entity);

		shapeRenderer->push_states(glm_aux::TS(transform.position + sphere.offset, glm::vec3(1.0f)));
		shapeRenderer->push_sphere_wireframe(sphere.radius, sphere.radius);
		shapeRenderer->pop_states<glm::mat4>();
	}

	shapeRenderer->pop_states<ShapeRendering::Color4u>();
}