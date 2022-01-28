#include "renderer.h"
#include <algorithm>
#include <algorithm> // std::fill
#include <cmath>
#include <functional>
#include <glm/common.hpp>
#include <glm/gtx/component_wise.hpp>
#include <iostream>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tuple>

namespace render {

// The renderer is passed a pointer to the volume, gradinet volume, camera and an initial renderConfig.
// The camera being pointed to may change each frame (when the user interacts). When the renderConfig
// changes the setConfig function is called with the updated render config. This gives the Renderer an
// opportunity to resize the framebuffer.
Renderer::Renderer(
    const volume::Volume* pVolume,
    const volume::GradientVolume* pGradientVolume,
    const render::RayTraceCamera* pCamera,
    const RenderConfig& initialConfig)
    : m_pVolume(pVolume)
    , m_pGradientVolume(pGradientVolume)
    , m_pCamera(pCamera)
    , m_config(initialConfig)
{
    resizeImage(initialConfig.renderResolution);
}

// Set a new render config if the user changed the settings.
void Renderer::setConfig(const RenderConfig& config)
{
    if (config.renderResolution != m_config.renderResolution)
        resizeImage(config.renderResolution);

    m_config = config;
}

// Resize the framebuffer and fill it with black pixels.
void Renderer::resizeImage(const glm::ivec2& resolution)
{
    m_frameBuffer.resize(size_t(resolution.x) * size_t(resolution.y), glm::vec4(0.0f));
}

// Clear the framebuffer by setting all pixels to black.
void Renderer::resetImage()
{
    std::fill(std::begin(m_frameBuffer), std::end(m_frameBuffer), glm::vec4(0.0f));
}

// Return a VIEW into the framebuffer. This view is merely a reference to the m_frameBuffer member variable.
// This does NOT make a copy of the framebuffer.
gsl::span<const glm::vec4> Renderer::frameBuffer() const
{
    return m_frameBuffer;
}

// Main render function. It computes an image according to the current renderMode.
// Multithreading is enabled in Release/RelWithDebInfo modes. In Debug mode multithreading is disabled to make debugging easier.
void Renderer::render()
{
    resetImage();

    static constexpr float sampleStep = 1.0f;
    const glm::vec3 planeNormal = -glm::normalize(m_pCamera->forward());
    const glm::vec3 volumeCenter = glm::vec3(m_pVolume->dims()) / 2.0f;
    const Bounds bounds { glm::vec3(0.0f), glm::vec3(m_pVolume->dims() - glm::ivec3(1)) };

    // 0 = sequential (single-core), 1 = TBB (multi-core)
#ifdef NDEBUG
    // If NOT in debug mode then enable parallelism using the TBB library (Intel Threaded Building Blocks).
#define PARALLELISM 1
#else
    // Disable multi threading in debug mode.
#define PARALLELISM 0
#endif

#if PARALLELISM == 0
    // Regular (single threaded) for loops.
    for (int x = 0; x < m_config.renderResolution.x; x++) {
        for (int y = 0; y < m_config.renderResolution.y; y++) {
#else
    // Parallel for loop (in 2 dimensions) that subdivides the screen into tiles.
    const tbb::blocked_range2d<int> screenRange { 0, m_config.renderResolution.y, 0, m_config.renderResolution.x };
        tbb::parallel_for(screenRange, [&](tbb::blocked_range2d<int> localRange) {
        // Loop over the pixels in a tile. This function is called on multiple threads at the same time.
        for (int y = std::begin(localRange.rows()); y != std::end(localRange.rows()); y++) {
            for (int x = std::begin(localRange.cols()); x != std::end(localRange.cols()); x++) {
#endif
            // Compute a ray for the current pixel.
            const glm::vec2 pixelPos = glm::vec2(x, y) / glm::vec2(m_config.renderResolution);
            Ray ray = m_pCamera->generateRay(pixelPos * 2.0f - 1.0f);

            // Compute where the ray enters and exists the volume.
            // If the ray misses the volume then we continue to the next pixel.
            if (!instersectRayVolumeBounds(ray, bounds))
                continue;

            // Get a color for the current pixel according to the current render mode.
            glm::vec4 color {};
            switch (m_config.renderMode) {
            case RenderMode::RenderSlicer: {
                color = traceRaySlice(ray, volumeCenter, planeNormal);
                break;
            }
            case RenderMode::RenderMIP: {
                color = traceRayMIP(ray, sampleStep);
                break;
            }
            case RenderMode::RenderComposite: {
                color = traceRayComposite(ray, sampleStep);
                break;
            }
            case RenderMode::RenderIso: {
                color = traceRayISO(ray, sampleStep);
                break;
            }
            case RenderMode::RenderTF2D: {
                color = traceRayTF2D(ray, sampleStep);
                break;
            }
            };
            // Write the resulting color to the screen.
            fillColor(x, y, color);

#if PARALLELISM == 1
        }
    }
});
#else
            }
        }
#endif
}

// ======= DO NOT MODIFY THIS FUNCTION ========
// This function generates a view alongside a plane perpendicular to the camera through the center of the volume
//  using the slicing technique.
glm::vec4 Renderer::traceRaySlice(const Ray& ray, const glm::vec3& volumeCenter, const glm::vec3& planeNormal) const
{
    const float t = glm::dot(volumeCenter - ray.origin, planeNormal) / glm::dot(ray.direction, planeNormal);
    const glm::vec3 samplePos = ray.origin + ray.direction * t;
    const float val = m_pVolume->getSampleInterpolate(samplePos);
    return glm::vec4(glm::vec3(std::max(val / m_pVolume->maximum(), 0.0f)), 1.f);
}

// ======= DO NOT MODIFY THIS FUNCTION ========
// Function that implements maximum-intensity-projection (MIP) raycasting.
// It returns the color assigned to a ray/pixel given it's origin, direction and the distances
// at which it enters/exits the volume (ray.tmin & ray.tmax respectively).
// The ray must be sampled with a distance defined by the sampleStep
glm::vec4 Renderer::traceRayMIP(const Ray& ray, float sampleStep) const
{
    float maxVal = 0.0f;

    // Incrementing samplePos directly instead of recomputing it each frame gives a measureable speed-up.
    glm::vec3 samplePos = ray.origin + ray.tmin * ray.direction;
    const glm::vec3 increment = sampleStep * ray.direction;
    for (float t = ray.tmin; t <= ray.tmax; t += sampleStep, samplePos += increment) {
        const float val = m_pVolume->getSampleInterpolate(samplePos);
        maxVal = std::max(val, maxVal);
    }

    // Normalize the result to a range of [0 to mpVolume->maximum()].
    return glm::vec4(glm::vec3(maxVal) / m_pVolume->maximum(), 1.0f);
}

// This function should find the position where the ray intersects with the volume's isosurface.
// If volume shading is DISABLED then simply return the isoColor.
// If volume shading is ENABLED then return the phong-shaded color at that location using the local gradient (from m_pGradientVolume).
//   Use the camera position (m_pCamera->position()) as the light position.
// Use the bisectionAccuracy function (to be implemented) to get a more precise isosurface location between two steps.
glm::vec4 Renderer::traceRayISO(const Ray& ray, float sampleStep) const
{
    const glm::vec3 light = m_pCamera->position();
    const glm::vec3 isoColor { 0.8f, 0.8f, 0.2f };
    const float isoVal = m_config.isoValue;

    // Incrementing samplePos directly instead of recomputing it each frame gives a measureable speed-up.
    glm::vec3 samplePos = ray.origin + ray.tmin * ray.direction;
    const glm::vec3 increment = sampleStep * ray.direction;

    for (float t = ray.tmin; t <= ray.tmax; t += sampleStep, samplePos += increment) {
        const float val = m_pVolume->getSampleInterpolate(samplePos);
        //isovalue crossed
        if (val >= isoVal) {
            //no shading
            if (!m_config.volumeShading)
                return glm::vec4(isoColor, 1.0f);

            float t_new = t; //use for bisection result
            //if t is not exactly isovalue, apply bisection algorithm
            if (val != isoVal)
                t_new = bisectionAccuracy(ray, t - 1.0f, t, isoVal);

            //new sampleposition for t_new
            glm::vec3 samplePos_t = ray.origin + t_new * ray.direction;

            const volume::GradientVoxel gradient = m_pGradientVolume->getGradientInterpolate(samplePos_t);

            glm::vec3 shading = computePhongShading(isoColor, gradient, light, ray.direction);
            return glm::vec4(shading, 1.0f);
        }
    }

    return glm::vec4(glm::vec3(0.0f), 1.0f);
}

// Given that the iso value lies somewhere between t0 and t1, find a t for which the value
// closely matches the iso value (less than 0.01 difference). Add a limit to the number of
// iterations such that it does not get stuck in degerate cases.
float Renderer::bisectionAccuracy(const Ray& ray, float t0, float t1, float isoValue) const
{
    static constexpr int max_iter = 10; //max iterations
    const float threshold = 0.01f; //difference threshold

    float t_left = t0;
    float t_right = t1;
    float t = (t_left + t_right) / 2;

    // Incrementing samplePos directly instead of recomputing it each frame gives a measureable speed-up.
    glm::vec3 samplePos = ray.origin + t * ray.direction;

    for (int i = 0; i < max_iter; i++) {
        t = (t_left + t_right) / 2;
        samplePos = ray.origin + t * ray.direction;
        const float val = m_pVolume->getSampleInterpolate(samplePos);

        //found t value
        if (abs(val - isoValue) < threshold)
            return t;

        //update search boundaries (left/right)
        if (val > isoValue)
            t_right = t;
        else
            t_left = t;
    }
    return t;
}

// Compute Phong Shading given the voxel color (material color), the gradient, the light vector and view vector.
// You can find out more about the Phong shading model at:
// https://en.wikipedia.org/wiki/Phong_reflection_model
//
// Use the given color for the ambient/specular/diffuse (you are allowed to scale these constants by a scalar value).
// You are free to choose any specular power that you'd like.
glm::vec3 Renderer::computePhongShading(const glm::vec3& color, const volume::GradientVoxel& gradient, const glm::vec3& L, const glm::vec3& V)
{
    //vectors
    const glm::vec3 n = glm::normalize(gradient.dir);
    const glm::vec3 L_norm = glm::normalize(L);
    const glm::vec3 R = (2 * glm::dot(n, L_norm)) * n - L_norm;

    //phong variables
    const glm::vec3 k = glm::vec3(.1f, .7f, .2f);
    const glm::vec3 I = color;
    const glm::vec3 S = I;

    const float alpha = 100.f;

    //cosine of angles between L, n and R, V
    const float cos_theta = glm::dot(L_norm, n) / (glm::length(L_norm) * glm::length(n));
    const float cos_phi = glm::dot(R, V) / (glm::length(R) * glm::length(V));

    //phong layers
    const glm::vec3 ambient = k.x * (I * S);
    const glm::vec3 diffuse = k.y * (I * S) * abs(cos_theta);
    const glm::vec3 specular = k.z * (I * S) * pow(cos_phi, alpha);

    return ambient + diffuse + specular;
}

// In this function, implement 1D transfer function raycasting.
// Use getTFValue to compute the color for a given volume value according to the 1D transfer function.
glm::vec4 Renderer::traceRayComposite(const Ray& ray, float sampleStep) const
{
    glm::vec4 C = glm::vec4(0); //composite vector initialised
    glm::vec3 samplePos = ray.origin + ray.tmax * ray.direction;
    const glm::vec3 increment = sampleStep * ray.direction;

    //back-to-front compositing
    for (float t = ray.tmax; t >= ray.tmin; t -= sampleStep, samplePos -= increment) {
        const float val = m_pVolume->getSampleInterpolate(samplePos);
        const glm::vec4 TFval = getTFValue(val);
        glm::vec3 color = glm::vec3(TFval.r, TFval.g, TFval.b);
        const float A = TFval.a; //opacity
        glm::vec4 C_i = glm::vec4(0); //position colour

        //if volume shading is applied
        if (m_config.volumeShading) {
            const glm::vec3 light = m_pCamera->position();
            const volume::GradientVoxel gradient = m_pGradientVolume->getGradientInterpolate(samplePos);

            //check whether the voxel is not 0
            //For some reason I can't implement this check in the computePhongShading function itself
            if (!glm::all(glm::equal(gradient.dir, glm::vec3(0)))) {
                glm::vec3 shading = computePhongShading(color, gradient, light, ray.direction);
                C_i = glm::vec4(shading * A, A);
            }
        } else {
            C_i = glm::vec4(color * A, A);
        }

        C = C_i + (1 - A) * C;
    }

    return C;
}

// ======= DO NOT MODIFY THIS FUNCTION ========
// Looks up the color+opacity corresponding to the given volume value from the 1D tranfer function LUT (m_config.tfColorMap).
// The value will initially range from (m_config.tfColorMapIndexStart) to (m_config.tfColorMapIndexStart + m_config.tfColorMapIndexRange) .
glm::vec4 Renderer::getTFValue(float val) const
{
    // Map value from [m_config.tfColorMapIndexStart, m_config.tfColorMapIndexStart + m_config.tfColorMapIndexRange) to [0, 1) .
    const float range01 = (val - m_config.tfColorMapIndexStart) / m_config.tfColorMapIndexRange;
    const size_t i = std::min(static_cast<size_t>(range01 * static_cast<float>(m_config.tfColorMap.size())), m_config.tfColorMap.size() - 1);
    return m_config.tfColorMap[i];
}

// ======= TODO: IMPLEMENT ========
// In this function, implement 2D transfer function raycasting.
// Use the getTF2DOpacity function that you implemented to compute the opacity according to the 2D transfer function.
glm::vec4 Renderer::traceRayTF2D(const Ray& ray, float sampleStep) const
{
    // Initializing vectors
    glm::vec4 C(0.0f);
    glm::vec3 Ci(0.0f);
    glm::vec4 colorVector = m_config.TF2DColor;

    float Ai = 0.0f;
    glm::vec3 samplePos = ray.origin + ray.tmin * ray.direction;
    const glm::vec3 increment = sampleStep * ray.direction;

    // Front to back compositing
    for (float t = ray.tmin; t <= ray.tmax; t += sampleStep, samplePos += increment) {

        if (Ai >= 0.95) {
            break;
        }

        const float val = m_pVolume->getSampleInterpolate(samplePos);
        const glm::vec4 TFval = getTFValue(val);
        volume::GradientVoxel gradient = m_pGradientVolume->getGradientInterpolate(samplePos);

        // Opacity
        float TFOpacity = getTF2DOpacity(val, gradient.magnitude);
        // Position colour
        const glm::vec3 currentColor(colorVector * TFOpacity);

        float x0 = currentColor[0] * (1 - Ai);
        float x1 = currentColor[1] * (1 - Ai);
        float x2 = currentColor[2] * (1 - Ai);
        glm::vec3 Ac(x0, x1, x2);

        Ci = Ci + Ac;
        Ai = Ai + (1 - Ai) * TFOpacity;
        C = glm::vec4(Ci, Ai);
    }

    return C;
}
// ======= TODO: IMPLEMENT ========
// This function should return an opacity value for the given intensity and gradient according to the 2D transfer function.
// Calculate whether the values are within the radius/intensity triangle defined in the 2D transfer function widget.
// If so: return a tent weighting as described in the assignment
// Otherwise: return 0.0f
//
// The 2D transfer function settings can be accessed through m_config.TF2DIntensity and m_config.TF2DRadius.
float Renderer::getTF2DOpacity(float intensity, float gradientMagnitude) const
{
    float TFIntensity = m_config.TF2DIntensity;
    float TFRadius = m_config.TF2DRadius;
    float A = 0.0;

    float slope = (m_pGradientVolume->maxMagnitude() / TFRadius);
    float input = abs(intensity - TFIntensity);

    // Inside of a triangle
    if (gradientMagnitude >= slope * input) {
        float factor = (input / (gradientMagnitude / slope));
        float interp_val = ((1 - factor) + (factor * 0));
        A = m_config.TF2DColor.a * interp_val;
    }
    return A;
}

// This function computes if a ray intersects with the axis-aligned bounding box around the volume.
// If the ray intersects then tmin/tmax are set to the distance at which the ray hits/exists the
// volume and true is returned. If the ray misses the volume the the function returns false.
//
// If you are interested you can learn about it at.
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool Renderer::instersectRayVolumeBounds(Ray& ray, const Bounds& bounds) const
{
    const glm::vec3 invDir = 1.0f / ray.direction;
    const glm::bvec3 sign = glm::lessThan(invDir, glm::vec3(0.0f));

    float tmin = (bounds.lowerUpper[sign[0]].x - ray.origin.x) * invDir.x;
    float tmax = (bounds.lowerUpper[!sign[0]].x - ray.origin.x) * invDir.x;
    const float tymin = (bounds.lowerUpper[sign[1]].y - ray.origin.y) * invDir.y;
    const float tymax = (bounds.lowerUpper[!sign[1]].y - ray.origin.y) * invDir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    tmin = std::max(tmin, tymin);
    tmax = std::min(tmax, tymax);

    const float tzmin = (bounds.lowerUpper[sign[2]].z - ray.origin.z) * invDir.z;
    const float tzmax = (bounds.lowerUpper[!sign[2]].z - ray.origin.z) * invDir.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    ray.tmin = std::max(tmin, tzmin);
    ray.tmax = std::min(tmax, tzmax);
    return true;
}

// This function inserts a color into the framebuffer at position x,y
void Renderer::fillColor(int x, int y, const glm::vec4& color)
{
    const size_t index = static_cast<size_t>(m_config.renderResolution.x * y + x);
    m_frameBuffer[index] = color;
}
}