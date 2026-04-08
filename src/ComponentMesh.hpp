#pragma once

#include <memory>

namespace eeng
{
    class RenderableMesh;
}

struct MeshComponent
{
    std::weak_ptr<eeng::RenderableMesh> mesh;
};