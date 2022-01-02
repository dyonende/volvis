#pragma once
#include <GL/glew.h> // Include before glfw3
#include <string_view>

GLuint loadShader(std::string_view source, GLenum type);
