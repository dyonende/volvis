#pragma once
#include "render/render_config.h"
#include "ui/transfer_func.h"
#include "ui/transfer_func_2d.h"
#include "volume/gradient_volume.h"
#include "volume/volume.h"
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace render {
class Renderer;
}

namespace ui {
class Menu {
public:
    Menu(const glm::ivec2& baseRenderResolution);

    using LoadVolumeCallback = std::function<void(const std::filesystem::path&)>;
    void setLoadVolumeCallback(LoadVolumeCallback&& callback);
    using RenderConfigChangedCallback = std::function<void(const render::RenderConfig&)>;
    void setRenderConfigChangedCallback(RenderConfigChangedCallback&& callback);
    using InterpolationModeChangedCallback = std::function<void(volume::InterpolationMode)>;
    void setInterpolationModeChangedCallback(InterpolationModeChangedCallback&& callback);

    render::RenderConfig renderConfig() const;
    volume::InterpolationMode interpolationMode() const;

    void setBaseRenderResolution(const glm::ivec2& baseRenderResolution);
    void setLoadedVolume(const volume::Volume& volume, const volume::GradientVolume& gradientVolume);

    void drawMenu(const glm::ivec2& pos, const glm::ivec2& size, std::chrono::duration<double> renderTime);

private:
    void showLoadVolTab();
    void showRayCastTab(std::chrono::duration<double> renderTime);
    void showTransFuncTab();
    void show2DTransFuncTab();

    void callRenderConfigChangedCallback() const;
    void callInterpolationModeChangedCallback() const;

private:
    bool m_volumeLoaded = false;
    std::string m_volumeInfo;
    int m_volumeMax;

    std::optional<TransferFunctionWidget> m_tfWidget;
    std::optional<TransferFunction2DWidget> m_tf2DWidget;

    glm::ivec2 m_baseRenderResolution;
    float m_resolutionScale { 1.0f };
    render::RenderConfig m_renderConfig {};
    volume::InterpolationMode m_interpolationMode { volume::InterpolationMode::NearestNeighbour };

    std::optional<LoadVolumeCallback> m_optLoadVolumeCallback;
    std::optional<RenderConfigChangedCallback> m_optRenderConfigChangedCallback;
    std::optional<InterpolationModeChangedCallback> m_optInterpolationModeChangedCallback;
};

}
