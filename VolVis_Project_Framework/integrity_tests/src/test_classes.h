#include <render/ray.h>
#include <render/renderer.h>
#include <volume/gradient_volume.h>
#include <volume/volume.h>
#include <utility>

#define provide_member_function_access(func_name)      \
    template <typename... Args>                        \
    auto test_##func_name(Args && ... args)          \
    {                                                  \
        return func_name(std::forward<Args>(args)...); \
    }
#define provide_const_member_function_access(func_name) \
    template <typename... Args>                         \
    auto test_##func_name(Args && ... args) const     \
    {                                                   \
        return func_name(std::forward<Args>(args)...);  \
    }

#define provide_static_member_function_access(func_name) \
    template <typename... Args>                          \
    static auto test_##func_name(Args && ... args)     \
    {                                                    \
        return func_name(std::forward<Args>(args)...);   \
    }

class TestVolume : public volume::Volume {
public:
    // Inherit constructor(s)
    using volume::Volume::Volume;

    provide_static_member_function_access(linearInterpolate)
    provide_const_member_function_access(getSampleTriLinearInterpolation)

    provide_static_member_function_access(weight)
    provide_static_member_function_access(cubicInterpolate)
    provide_const_member_function_access(biCubicInterpolate)
    provide_const_member_function_access(getSampleTriCubicInterpolation)
};

class TestGradientVolume : public volume::GradientVolume {
public:
    using volume::GradientVolume::GradientVolume;

    provide_static_member_function_access(linearInterpolate)
    provide_const_member_function_access(getGradientLinearInterpolate)
};

class TestRenderer : public render::Renderer {
public:
    using render::Renderer::Renderer;

    provide_member_function_access(traceRaySlice)
    provide_member_function_access(traceRayMIP)
    provide_member_function_access(traceRayISO)
    provide_member_function_access(traceRayComposite)
    provide_member_function_access(traceRayTF2D)

    provide_member_function_access(bisectionAccuracy)
    provide_member_function_access(computePhongShading)
};