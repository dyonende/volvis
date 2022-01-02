#include "ui/full_screen_texture_gl.h"
#include "opengl.h"
#include "ui/gl_error.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>

namespace ui {

FullScreenTextureGL::FullScreenTextureGL()
{
    // Generate texture
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    /*glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			resolution.x,
			resolution.y,
			0,
			GL_RGBA,
			GL_FLOAT,
			nullptr);*/
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create full screen quad
    // For whatever reason, OpenGL doesnt like GL_QUADS (gives me no output), so I'll just use two triangles
    // NOTE: vertical texture coordinates are swapped
    // clang-format off
	float vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 0.0f,

		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 0.0f, 0.0f
	};
    // clang-format on
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glBindVertexArray(0);

    // Load shader
    {
        GLuint vertexShader = loadShader("viewer_output.vs", GL_VERTEX_SHADER);
        GLuint fragmentShader = loadShader("viewer_output.fs", GL_FRAGMENT_SHADER);

        m_shader = glCreateProgram();
        glAttachShader(m_shader, vertexShader);
        glAttachShader(m_shader, fragmentShader);
        glLinkProgram(m_shader);

        glDetachShader(m_shader, vertexShader);
        glDetachShader(m_shader, fragmentShader);
    }

    const glm::vec3 black { 0.0f };
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 1, 1, 0, GL_RGB, GL_FLOAT, glm::value_ptr(black));
    glBindTexture(GL_TEXTURE_2D, 0);
}

FullScreenTextureGL::~FullScreenTextureGL()
{
    glDeleteTextures(1, &m_texture);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_shader);
}

void FullScreenTextureGL::update(gsl::span<const glm::vec3> frameBuffer, const glm::ivec2& resolution)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, resolution.x, resolution.y, 0, GL_RGB, GL_FLOAT, frameBuffer.data());
}

void FullScreenTextureGL::update(gsl::span<const glm::vec4> frameBuffer, const glm::ivec2& resolution)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, resolution.x, resolution.y, 0, GL_RGBA, GL_FLOAT, frameBuffer.data());
}

void FullScreenTextureGL::draw()
{
    glUseProgram(m_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(glGetUniformLocation(m_shader, "u_texture"), 0);

    glBindVertexArray(m_vao);

    //glEnable(GL_FRAMEBUFFER_SRGB);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //glDisable(GL_FRAMEBUFFER_SRGB);
}

}
