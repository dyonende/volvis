#pragma once
#include <glm/mat4x4.hpp>

namespace ui {

class RasterizationCamera {
public:
    virtual ~RasterizationCamera() = default;

    virtual glm::mat4 viewMatrix() const = 0;
    virtual glm::mat4 projectionMatrix() const = 0;
};

}