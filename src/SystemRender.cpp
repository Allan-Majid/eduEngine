#include "SystemRender.hpp"
#include "ComponentTransform.hpp"
#include "ComponentMesh.hpp"
#include "glmcommon.hpp"

void RenderSystem::Render(entt::registry& registry, eeng::ForwardRendererPtr renderer, float time)
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
			transform.rotation.y, { 0, 1, 0 },
			transform.scale
		);

		renderer->renderMesh(meshPtr, worldMatrix);

	}
}