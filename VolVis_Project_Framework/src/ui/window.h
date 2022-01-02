#pragma once
#include <GL/glew.h> // Include before glfw3
#include <GLFW/glfw3.h>
#include <functional>
#include <glm/vec2.hpp>
#include <optional>
#include <string_view>
#include <vector>

namespace ui {

class Window {
public:
    Window(std::string_view title, const glm::ivec2& windowSize, bool legacyGL = false);
    ~Window();

    void close(); // Set shouldClose() to true.
    [[nodiscard]] bool shouldClose(); // Whether window should close (close() was called or user clicked the close button).

    void updateInput();
    void swapBuffers(); // Swap the front/back buffer

    using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
    void registerKeyCallback(KeyCallback&&);
    using MouseButtonCallback = std::function<void(int button, int action, int mods)>;
    void registerMouseButtonCallback(MouseButtonCallback&&);
    using MouseMoveCallback = std::function<void(const glm::vec2& cursorPos)>;
    void registerMouseMoveCallback(MouseMoveCallback&&);
    using ScrollCallback = std::function<void(const glm::vec2& offset)>;
    void registerScrollCallback(ScrollCallback&&);
    using WindowResizeCallback = std::function<void(const glm::ivec2& size)>;
    void registerWindowResizeCallback(WindowResizeCallback&&);

    bool isKeyPressed(int key) const;
    bool isMouseButtonPressed(int button) const;
    glm::vec2 cursorPos() const;
    glm::vec2 normalizedCursorPos() const; // Ranges from 0 to 1.

    // Hides mouse and prevents it from going out of the window.
    // Usefull for a first person camera.
    void setMouseCapture(bool capture);

    [[nodiscard]] glm::ivec2 windowSize() const;
    [[nodiscard]] glm::ivec2 frameBufferResolution() const;
    [[nodiscard]] float aspectRatio() const;

private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned unicodeCodePoint);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);

private:
    GLFWwindow* m_pWindow;
    glm::ivec2 m_windowSize;

    std::vector<KeyCallback> m_keyCallbacks;
    std::vector<MouseButtonCallback> m_mouseButtonCallbacks;
    std::vector<ScrollCallback> m_scrollCallbacks;
    std::vector<MouseMoveCallback> m_mouseMoveCallbacks;
    std::vector<WindowResizeCallback> m_windowResizeCallbacks;
};

}
