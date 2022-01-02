#pragma once
#include "opengl.h"
#include "ui/trackball.h"

namespace ui {

class SurfaceCube {
public:
    SurfaceCube();
    ~SurfaceCube();

    void draw(const Trackball& camera, const glm::vec3& scale);

private:
    GLuint m_ibo, m_vbo, m_vao;
    GLuint m_shader;
};
}
