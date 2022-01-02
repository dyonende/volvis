#pragma once
#include "ui/opengl.h"
#include "ui/rasterization_camera.h"
#include <glm/vec3.hpp>

namespace ui {

class WireframeCube {
public:
    WireframeCube();
    ~WireframeCube();

    void draw(const ui::RasterizationCamera& camera, const glm::vec3& scale, const glm::vec3& offset, const glm::vec3& color);

private:
    GLuint m_ibo, m_vbo, m_vao;
    GLuint m_shader;
};
}
