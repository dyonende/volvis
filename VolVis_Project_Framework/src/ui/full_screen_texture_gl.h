#pragma once
#include "ui/window.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <gsl/span>

namespace ui {

class FullScreenTextureGL {
public:
    FullScreenTextureGL();
    ~FullScreenTextureGL();

    void update(gsl::span<const glm::vec3> frameBuffer, const glm::ivec2& resolution);
    void update(gsl::span<const glm::vec4> frameBuffer, const glm::ivec2& resolution);
    void draw();

private:
    GLuint m_texture;
    GLuint m_vbo, m_vao;
    GLuint m_shader;
};
}
