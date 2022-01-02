#include "surface_cube.h"
#include <array>
#include <glm/gtc/type_ptr.hpp>

namespace ui {

SurfaceCube::SurfaceCube()
{
    // https://people.sc.fsu.edu/~jburkardt/data/obj/cube.obj
    // clang-format off
    const std::array vertices {
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,
    };
    const std::array<unsigned, 36> indices {
        0,  6,  4,
        0,  2,  6,
        0,  3,  2,
        0,  1,  3,
        2,  7,  6,
        2,  3,  7,
        4,  6,  7,
        4,  7,  5,
        0,  4,  5,
        0,  5,  1,
        1,  5,  7,
        1,  7,  3
    };
    // clang-format on

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

    /*glNamedBufferStorage(m_ibo, GLsizeiptr(indices.size() * sizeof(decltype(indices)::value_type)), indices.data(), 0);
		glGenBuffers(1, &m_vbo);
		glNamedBufferStorage(m_vbo, 4*3*sizeof(float), vertices.data(), 0);

		glCreateVertexArrays(1, &m_vao);
		glVertexArrayElementBuffer(m_vao, m_ibo);
		glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 3*sizeof(float));
		glEnableVertexArrayAttrib(m_vao, 0);*/

    // Load shader
    {
        GLuint vertexShader = loadShader("surface_cube.vs", GL_VERTEX_SHADER);
        GLuint fragmentShader = loadShader("surface_cube.fs", GL_FRAGMENT_SHADER);

        m_shader = glCreateProgram();
        glAttachShader(m_shader, vertexShader);
        glAttachShader(m_shader, fragmentShader);
        glLinkProgram(m_shader);

        glDetachShader(m_shader, vertexShader);
        glDetachShader(m_shader, fragmentShader);
    }
}

SurfaceCube::~SurfaceCube()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_ibo);
    glDeleteBuffers(1, &m_vbo);
}

void ui::SurfaceCube::draw(const Trackball& camera, const glm::vec3& scale)
{
    const auto modelMatrix = glm::scale(glm::identity<glm::mat4>(), scale);
    const auto viewMatrix = camera.viewMatrix();
    const auto projectionMatrix = camera.projectionMatrix();
    auto viewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

    glUseProgram(m_shader);
    glBindVertexArray(m_vao);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_modelViewProjection"), 1, false, glm::value_ptr(viewProjectionMatrix));
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}
}
