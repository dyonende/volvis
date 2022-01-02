#pragma once
#include "render/ray.h"
#include "render/ray_trace_camera.h"
#include "render/render_config.h"
#include "volume/gradient_volume.h"
#include "volume/volume.h"
#include <cstring> // memcmp
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <gsl/span>
#include <memory>
#include <tuple>
#include <vector>

namespace render {

union Bounds {
    struct {
        glm::vec3 lower;
        glm::vec3 upper;
    } IndividualBounds; // macOS change TH
    std::array<glm::vec3, 2> lowerUpper;
};

class Renderer {
public:
    Renderer(
        const volume::Volume* pVolume,
        const volume::GradientVolume* pGradientVolume,
        const render::RayTraceCamera* pCamera,
        const RenderConfig& config);

    void setConfig(const RenderConfig& config);
    void render();
    gsl::span<const glm::vec4> frameBuffer() const;

protected:
    // These functions will be automatically tested.
    glm::vec4 traceRaySlice(const Ray& ray, const glm::vec3& volumeCenter, const glm::vec3& planeNormal) const;
    glm::vec4 traceRayMIP(const Ray& ray, float sampleStep) const;
    glm::vec4 traceRayISO(const Ray& ray, float sampleStep) const;
    glm::vec4 traceRayComposite(const Ray& ray, float sampleStep) const;
    glm::vec4 traceRayTF2D(const Ray& ray, float sampleStep) const;

    float bisectionAccuracy(const Ray& ray, float t0, float t1, float isoValue) const;

    static glm::vec3 computePhongShading(const glm::vec3& color, const volume::GradientVoxel& gradient, const glm::vec3& lightDirection, const glm::vec3& viewDirection);

private:
    void resizeImage(const glm::ivec2& resolution);
    void resetImage();

    glm::vec4 getTFValue(float val) const;
    float getTF2DOpacity(float val, float gradientMagnitude) const;

    bool instersectRayVolumeBounds(Ray& ray, const Bounds& volumeBounds) const;
    void fillColor(int x, int y, const glm::vec4& color);

protected:
    const volume::Volume* m_pVolume;
    const volume::GradientVolume* m_pGradientVolume;
    const render::RayTraceCamera* m_pCamera;
    RenderConfig m_config;

    std::vector<glm::vec4> m_frameBuffer;
};

}
