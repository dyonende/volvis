#include "window.h"
#include <imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <iostream>

static void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW error code: " << error << std::endl;
    std::cerr << description << std::endl;
    exit(1);
}

#ifdef GL_DEBUG_SEVERITY_NOTIFICATION
// OpenGL debug callback
void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::cerr << "OpenGL: " << message << std::endl;
    }
}
#endif

namespace ui {

Window::Window(std::string_view title, const glm::ivec2& windowSize, bool legacyGL)
    : m_windowSize(windowSize)
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW" << std::endl;
        exit(1);
    }

    if (!legacyGL) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
#ifndef NDEBUG // Automatically defined by CMake when compiling in Release/MinSizeRel mode.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
    // std::string_view does not guarantee that the string contains a terminator character.
    const std::string titleString { title };
    m_pWindow = glfwCreateWindow(windowSize.x, windowSize.y, titleString.c_str(), nullptr, nullptr);
    if (m_pWindow == nullptr) {
        glfwTerminate();
        std::cerr << "Could not create GLFW window" << std::endl;
        exit(1);
    }
    glfwMakeContextCurrent(m_pWindow);
    glfwSwapInterval(1); // Enable vsync. To disable vsync set this to 0.

    glewInit();

    int glVersionMajor, glVersionMinor;
    glGetIntegerv(GL_MAJOR_VERSION, &glVersionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &glVersionMinor);
    std::cout << "Initialized OpenGL version " << glVersionMajor << "." << glVersionMinor << std::endl;

#if !defined(NDEBUG)
    // Custom debug message with breakpoints at the exact error. Only supported on OpenGL 4.1 and higher.
    if (glVersionMajor > 4 || (glVersionMajor == 4 && glVersionMinor >= 3)) {
        // Set OpenGL debug callback when supported (OpenGL 4.3).
        // NOTE(Mathijs): this is not supported on macOS since Apple can't be bothered to update
        //  their OpenGL version past 4.1 which released 10 years ago!
        glDebugMessageCallback(glDebugCallback, nullptr);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
#endif

    // Setup Dear ImGui context.
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_pWindow, false);
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "Could not initialize imgui" << std::endl;
        exit(1);
    }

    glfwSetWindowUserPointer(m_pWindow, this);

    glfwSetKeyCallback(m_pWindow, keyCallback);
    glfwSetCharCallback(m_pWindow, charCallback);
    glfwSetMouseButtonCallback(m_pWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(m_pWindow, mouseMoveCallback);
    glfwSetScrollCallback(m_pWindow, scrollCallback);
    glfwSetWindowSizeCallback(m_pWindow, windowSizeCallback);
}

Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();
}

void Window::close()
{
    glfwSetWindowShouldClose(m_pWindow, 1);
}

bool Window::shouldClose()
{
    return glfwWindowShouldClose(m_pWindow) != 0;
}

void Window::updateInput()
{
    glfwPollEvents();

    // Start the Dear ImGui frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Window::swapBuffers()
{
    // Rendering of Dear ImGui ui.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_pWindow);
}

void Window::registerKeyCallback(KeyCallback&& callback)
{
    m_keyCallbacks.push_back(std::move(callback));
}

void Window::registerMouseButtonCallback(MouseButtonCallback&& callback)
{
    m_mouseButtonCallbacks.push_back(std::move(callback));
}

void Window::registerScrollCallback(ScrollCallback&& callback)
{
    m_scrollCallbacks.push_back(std::move(callback));
}

void Window::registerWindowResizeCallback(WindowResizeCallback&& callback)
{
    m_windowResizeCallbacks.push_back(std::move(callback));
}

void Window::registerMouseMoveCallback(MouseMoveCallback&& callback)
{
    m_mouseMoveCallbacks.push_back(std::move(callback));
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Ignore callbacks when the user is interacting with imgui.
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_keyCallbacks)
        callback(key, scancode, action, mods);
}

void Window::charCallback(GLFWwindow* window, unsigned unicodeCodePoint)
{
    ImGui_ImplGlfw_CharCallback(window, unicodeCodePoint);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Ignore callbacks when the user is interacting with imgui.
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_mouseButtonCallbacks)
        callback(button, action, mods);
}

void Window::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Ignore callbacks when the user is interacting with imgui.
    if (ImGui::GetIO().WantCaptureMouse)
        return;


    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_mouseMoveCallbacks)
        callback(glm::vec2(xpos, ypos));
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Ignore callbacks when the user is interacting with imgui.
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_scrollCallbacks)
        callback(glm::vec2(xoffset, yoffset));
}

void Window::windowSizeCallback(GLFWwindow* window, int width, int height)
{
    Window* pThisWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    pThisWindow->m_windowSize = glm::ivec2 { width, height };
    
    for (const auto& callback : pThisWindow->m_windowResizeCallbacks)
        callback(glm::ivec2(width, height));
}

bool Window::isKeyPressed(int key) const
{
    return glfwGetKey(m_pWindow, key) == GLFW_PRESS;
}

bool Window::isMouseButtonPressed(int button) const
{
    return glfwGetMouseButton(m_pWindow, button) == GLFW_PRESS;
}

glm::vec2 Window::cursorPos() const
{
    double x, y;
    glfwGetCursorPos(m_pWindow, &x, &y);
    return glm::vec2(x, y);
}

glm::vec2 Window::normalizedCursorPos() const
{
    return cursorPos() / glm::vec2(m_windowSize);
}

void Window::setMouseCapture(bool capture)
{
    if (capture) {
        glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    glfwPollEvents();
}

glm::ivec2 Window::windowSize() const
{
    return m_windowSize;
}

glm::ivec2 Window::frameBufferResolution() const
{
    glm::ivec2 out;
    glfwGetFramebufferSize(m_pWindow, &out.x, &out.y);
    return out;
}

float Window::aspectRatio() const
{
    return float(m_windowSize.x) / float(m_windowSize.y);
}
}
