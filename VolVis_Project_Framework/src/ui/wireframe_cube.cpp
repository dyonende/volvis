#include "wireframe_cube.h"
#include <array>
#include <glm/gtc/type_ptr.hpp>

namespace ui {
WireframeCube::WireframeCube()
{
    const std::array vertices {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };
    const std::array<unsigned, 24> indices {
        0, 1, 0, 2, 0, 4,
        6, 2, 6, 4, 6, 7,
        3, 2, 3, 1, 3, 7,
        5, 1, 5, 7, 5, 4
    };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);

    // Load shader
    {
        GLuint vertexShader = loadShader("wireframe_cube.vs", GL_VERTEX_SHADER);
        GLuint fragmentShader = loadShader("wireframe_cube.fs", GL_FRAGMENT_SHADER);

        m_shader = glCreateProgram();
        glAttachShader(m_shader, vertexShader);
        glAttachShader(m_shader, fragmentShader);
        glLinkProgram(m_shader);

        glDetachShader(m_shader, vertexShader);
        glDetachShader(m_shader, fragmentShader);
    }
}

WireframeCube::~WireframeCube()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_ibo);
    glDeleteBuffers(1, &m_vbo);
}

void ui::WireframeCube::draw(const ui::RasterizationCamera& camera, const glm::vec3& scale, const glm::vec3& offset, const glm::vec3& color)
{
    const auto modelMatrix = glm::scale(glm::translate(glm::identity<glm::mat4>(), offset), scale);
    const auto viewMatrix = camera.viewMatrix();
    const auto projectionMatrix = camera.projectionMatrix();
    auto viewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

    glUseProgram(m_shader);
    glBindVertexArray(m_vao);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_modelViewProjection"), 1, false, glm::value_ptr(viewProjectionMatrix));
    glUniform3fv(glGetUniformLocation(m_shader, "u_color"), 1, glm::value_ptr(color));
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
}
}
