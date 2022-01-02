#include "trackball.h"
#include "window.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <limits>

static constexpr float rotationSpeedFactor = 0.3f;
static constexpr float translationSpeedFactor = 0.002f;
static constexpr float zoomSpeedFactor = 0.1f;

namespace ui {
Trackball::Trackball(Window* pWindow, float fovy, float aspectRatio, float dist)
    : m_pWindow(pWindow)
    , m_fovy(fovy)
    , m_aspectRatio(aspectRatio)
    , m_distanceFromLookAt(dist)
{
    pWindow->registerMouseButtonCallback(
        [this](int key, int action, int mods) {
            mouseButtonCallback(key, action, mods);
        });
    pWindow->registerMouseMoveCallback(
        [this](const glm::vec2& pos) {
            mouseMoveCallback(pos);
        });
    pWindow->registerScrollCallback(
        [this](const glm::vec2& offset) {
            mouseScrollCallback(offset);
        });

    updateCameraPos();
}

// Print the camera controls to the console
void Trackball::printHelp()
{
    std::cout << "Left button: turn in XY," << std::endl;
    std::cout << "Right button: translate in XY," << std::endl;
    std::cout << "Middle button: move along Z." << std::endl;
}

// Set look at and recompute cameraPos
void Trackball::setLookAt(const glm::vec3& lookAt)
{
    m_lookAt = lookAt;
    updateCameraPos();
}

// Set distance and recompute cameraPos
void Trackball::setDistance(float distance)
{
    m_distanceFromLookAt = distance;
    updateCameraPos();
}

// More getters and setters

void Trackball::setWorldScale(float scale)
{
    m_worldScale = scale;
}

glm::vec3 Trackball::position() const
{
    return m_cameraPos;
}

glm::mat4 Trackball::viewMatrix() const
{
    return glm::lookAt(m_cameraPos, m_lookAt, up());
}

glm::mat4 Trackball::projectionMatrix() const
{
    return glm::perspective(m_fovy, m_aspectRatio, 10.0f, 1000.0f);
}

glm::vec3 Trackball::up() const
{
    return m_rotation * glm::vec3(0, 1, 0);
}

glm::vec3 Trackball::left() const
{
    return m_rotation * glm::vec3(-1, 0, 0);
}

glm::vec3 Trackball::forward() const
{
    return m_rotation * glm::vec3(0, 0, 1);
}

// This function generates a ray with its origin at cameraPos, going through pixel pixel on the virtual screen
render::Ray Trackball::generateRay(const glm::vec2& pixel) const
{
    const float halfScreenPlaceHeight = std::tan(m_fovy / 2.0f);
    const float halfScreenPlaceWidth = m_aspectRatio * halfScreenPlaceHeight;
    const glm::vec3 cameraSpaceDirection = glm::normalize(glm::vec3(pixel.x * halfScreenPlaceWidth, pixel.y * halfScreenPlaceHeight, 1.0f));

    render::Ray ray;
    ray.origin = m_cameraPos;
    ray.direction = m_rotation * cameraSpaceDirection;
    ray.tmin = std::numeric_limits<float>::lowest();
    ray.tmax = std::numeric_limits<float>::max();
    return ray;
}

// This function handles mouse button interaction, where the type of movement depends on
//  the button pressed
void Trackball::mouseButtonCallback(int button, int action, int /* mods */)
{
    if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS) {
        m_prevCursorPos = m_pWindow->cursorPos();
    }
}

// This function computes the new camera position and orientation when the mouse is moved
void Trackball::mouseMoveCallback(const glm::vec2& pos)
{
    const bool rotateXY = m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool translateXY = m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

    if (rotateXY || translateXY) {
        // motion amount.
        glm::vec2 delta = pos - m_prevCursorPos;
        delta.y = -delta.y; // Vertical axis direction is inverted.

        if (rotateXY) {
            // Rotate model.
            m_rotation = glm::angleAxis(glm::radians(-delta.x * rotationSpeedFactor), up()) * m_rotation;
            m_rotation = glm::angleAxis(glm::radians(delta.y * rotationSpeedFactor), left()) * m_rotation;
            m_rotation = glm::normalize(m_rotation); // Prevent floating point drift from accumulating over time.
        } else {
            m_lookAt -= delta.x * m_worldScale * translationSpeedFactor * left();
            m_lookAt -= delta.y * m_worldScale * translationSpeedFactor * up();
        }
        m_prevCursorPos = pos;

        updateCameraPos();
    }
}

// This function computes the new camera position when we zoom using the scroll wheel
void Trackball::mouseScrollCallback(const glm::vec2& offset)
{
    m_distanceFromLookAt += -float(offset.y) * m_worldScale * zoomSpeedFactor;
    m_distanceFromLookAt = glm::max(0.0f, m_distanceFromLookAt);

    updateCameraPos();
}

void Trackball::updateCameraPos()
{
    m_cameraPos = m_lookAt + m_rotation * glm::vec3(0, 0, -m_distanceFromLookAt);
}

}
