#pragma once
#include "ray.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace render {

class RayTraceCamera {
public:
    virtual ~RayTraceCamera() = default;

    virtual glm::vec3 position() const = 0;
    virtual glm::vec3 forward() const = 0;

    virtual render::Ray generateRay(const glm::vec2& pixel) const = 0;
};

}