#pragma once
#include "render/ray.h"
#include "render/ray_trace_camera.h"
#include "ui/rasterization_camera.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace ui {
class Window;

class Trackball : public render::RayTraceCamera, public ui::RasterizationCamera {
public:
    Trackball(Window* pWindow, float fovy, float aspectRatio, float dist = 4.0f);
    ~Trackball() override = default;

    static void printHelp();

    void setLookAt(const glm::vec3& lookAt);
    void setDistance(float distance);
    void setWorldScale(float scale);

    glm::vec3 position() const override;
    glm::mat4 viewMatrix() const override;
    glm::mat4 projectionMatrix() const override;

    glm::vec3 left() const;
    glm::vec3 up() const;
    glm::vec3 forward() const override;

    // Generate ray given pixel in NDC space (-1 to +1)
    render::Ray generateRay(const glm::vec2& pixel) const override;

private:
    void mouseButtonCallback(int button, int action, int mods);
    void mouseMoveCallback(const glm::vec2& pos);
    void mouseScrollCallback(const glm::vec2& offset);

    void updateCameraPos();

private:
    const Window* m_pWindow;
    float m_fovy, m_aspectRatio;

    // Indication of world scale used to scale zoom / translate speed.
    float m_worldScale { 1.0f };

    glm::vec3 m_cameraPos { 0.0f };
    glm::vec3 m_lookAt { 0.0f };
    float m_distanceFromLookAt;
    glm::quat m_rotation { glm::identity<glm::quat>() };

    glm::vec2 m_prevCursorPos; // Cursor position.
};
}
