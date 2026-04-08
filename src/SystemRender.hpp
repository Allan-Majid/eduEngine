#pragma once

#include <entt/entt.hpp>
#include "ForwardRenderer.hpp"

class RenderSystem
{
public:
	void Render(entt::registry& registry, eeng::ForwardRendererPtr renderer, float time);
};