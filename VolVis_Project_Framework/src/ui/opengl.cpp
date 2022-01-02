#include "opengl.h"
#include <fstream>
#include <string>
#include <iostream>

GLuint loadShader(std::string_view fileName, GLenum type)
{
	std::ifstream file = std::ifstream(std::string(fileName));
	if (!file.is_open()) {
		std::string errorMessage = "Cannot open file: ";
		errorMessage += fileName;
		std::cerr << errorMessage.c_str() << std::endl;
	}
	const std::string source(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));

	const GLuint shader = glCreateShader(type);
	const char* sourcePtr = source.data();
	const GLint sourceSize = static_cast<GLint>(source.length());
	glShaderSource(shader, 1, &sourcePtr, &sourceSize);
	glCompileShader(shader);

	GLint shaderCompiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
	if (!shaderCompiled) {
		std::cerr << "Error compiling " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader" << std::endl;
		int infologLength = 0;
		int charsWritten = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 0) {
			std::string infoLog;
			infoLog.resize(static_cast<size_t>(infologLength));
			glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog.data());
			std::cout << infoLog << std::endl;
		}
	}
	return shader;
}
