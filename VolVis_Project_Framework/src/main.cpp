
// imgui has to be included before imgui_impl_glfw.h or imgui_impl_opengl3.h
#include <imgui.h>

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "render/renderer.h"
#include "ui/full_screen_texture_gl.h"
#include "ui/menu.h"
#include "ui/surface_cube.h"
#include "ui/trackball.h"
#include "ui/window.h"
#include "ui/wireframe_cube.h"
#include "volume/gradient_volume.h"
#include "volume/volume.h"
#include <chrono>
#include <cmath> // log2
#include <glm/geometric.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <iostream>
#include <optional>
#include <ratio>
#include <vector>

int main(int argc, char** argv)
{
    // NOTE: This is the size in DPI independent window units.
    constexpr int menuWidth = 560;
    glm::ivec2 viewportSize { 720, 720 };
    glm::ivec2 windowSize { viewportSize.x + menuWidth, viewportSize.y };
    constexpr float frameTimeTarget = 1.0f / 60.0f; // Target 60 fps.

    // === VIEWER ===
    ui::Window myWindow { "VolVis Viewer", windowSize };
    // Get DPI aware rendering resolution after creating the Window.
    const glm::vec2 dpiScaling = myWindow.frameBufferResolution() / windowSize;
    glm::ivec2 baseRenderResolution = glm::ivec2(glm::vec2(viewportSize));
    // The window may be shrunk by at most half the rendering resolution.
    //myWindow.setMinWindowSize(glm::ivec2(baseRenderResolution.x / 2 + menuWidth, baseRenderResolution.y / 2));

    const float aspectRatio = static_cast<float>(viewportSize.x) / static_cast<float>(viewportSize.y);
    ui::Trackball trackballCamera { &myWindow, glm::radians(60.0f), aspectRatio };

    // Render instance contains everything you need to render (volume + renderer). Initially there is
    // nothing to render hence the optional (initially it is empty). The optional is passed to the menu
    // class which is responsible for creating the volume + renderer when the user loads a volume.
    std::optional<volume::Volume> optVolume;
    std::optional<volume::GradientVolume> optGradientVolume;
    std::optional<render::Renderer> optRenderer;
    ui::Menu volVisMenu { viewportSize };

    // Whether to redraw because the user interacted with the application. When this is the reason for the
    // redraw then dynamic resolution scaling is enabled. After the user interaction, one more render is
    // performed at the full (selected) resolution. When the application is static no renders are performed.
    bool redrawUserInteraction = false;
    bool redrawFullResolution = true;
    auto loadVolume = [&](const std::filesystem::path& filePath) {
        optVolume.emplace(filePath.string());
        optVolume->interpolationMode = volVisMenu.interpolationMode();
        optGradientVolume.emplace(optVolume.value());
        optRenderer.emplace(&optVolume.value(), &optGradientVolume.value(), &trackballCamera, volVisMenu.renderConfig());

        const float maxDimension = float(glm::compMax(optVolume->dims()));
        trackballCamera.setDistance(maxDimension);
        trackballCamera.setWorldScale(maxDimension);
        trackballCamera.setLookAt(glm::vec3(optVolume->dims()) / 2.0f);

        volVisMenu.setLoadedVolume(optVolume.value(), optGradientVolume.value());

        redrawUserInteraction = true;
    };

    // Callbacks.
    volVisMenu.setLoadVolumeCallback(loadVolume);
    volVisMenu.setRenderConfigChangedCallback(
        [&](const render::RenderConfig& renderConfig) {
            if (optRenderer)
                optRenderer->setConfig(renderConfig);
            redrawUserInteraction = true;
        });
    volVisMenu.setInterpolationModeChangedCallback(
        [&](volume::InterpolationMode interpolationMode) {
            if (optVolume) {
                optVolume->interpolationMode = interpolationMode;
                optGradientVolume->interpolationMode = interpolationMode;
            }
            redrawUserInteraction = true;
        });
    myWindow.registerWindowResizeCallback(
        [&](const glm::ivec2& newWindowSize) {
            // Maintain aspect ratio!
            const int potentialWidth = newWindowSize.x - menuWidth;
            const int potentialHeight = newWindowSize.y;
            viewportSize = glm::ivec2(std::min(potentialWidth, potentialHeight));
            baseRenderResolution = glm::ivec2(glm::vec2(viewportSize) * dpiScaling);
            volVisMenu.setBaseRenderResolution(baseRenderResolution);
            windowSize = newWindowSize;
            redrawUserInteraction = true;
        });

    // Create GPU side texture.
    ui::FullScreenTextureGL fullScreenTextureGL;
    ui::WireframeCube wireframeCube;
    ui::SurfaceCube surfaceCube;

    // The dynamic resolution scale that was used in previous frame (to keep the frame time below the target).
    int prevResolutionScale = 1;
    std::chrono::duration<double> renderTime { 0 };
    while (!myWindow.shouldClose()) {
        myWindow.updateInput();

        if (optRenderer.has_value()) {
            // If camera changed in any way then we need to redraw.
            static glm::mat4 prevViewMatrix = glm::identity<glm::mat4>();
            const glm::mat4 viewMatrix = trackballCamera.viewMatrix();
            if (prevViewMatrix != viewMatrix) {
                prevViewMatrix = viewMatrix;
                redrawUserInteraction = true;
            }
            // If previous frame we rendered at a lower resolution (because something changed) then it will request to draw
            // the next frame in full resolution. If the user is still holding the mouse button then we can reasonably assume
            // that (s)he is not finished with the interaction (so we should keep rendering at a lower resolution).
            if (redrawFullResolution && (myWindow.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || myWindow.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)))
                redrawUserInteraction = true;

            // We draw when either the user has interacted (camera matrix changed or render config changed (see callback)) or if
            //  last frame we rendered at a lower resolution and we want to now render at the full resolution.
            if (redrawUserInteraction || redrawFullResolution) {
                if (redrawUserInteraction) {
                    // Reduce the resolution if the performance drops below the target frame time.
                    // Estimated performance when rendering at full resolution (resolution returned from menu).
                    // This way we can dynamically update the resolution while the user is moving the camera since
                    // some views may be slower to render than others.
                    const float estimatedFullResFrameTime = float(renderTime.count()) * float(prevResolutionScale * prevResolutionScale);
                    const float performanceScale = estimatedFullResFrameTime / float(frameTimeTarget);
                    // Resolution scale changes the number of pixels quadratically (scales both width and height).
                    const int resolutionScale = std::max(int(std::sqrt(performanceScale)) + 1, 1);

                    // NOTE(Mathijs): calling setBaseRenderResolution will update the render config and call
                    //  the associated callback. Make sure that you don't read redrawUserInteraction after
                    //  this call because it will always be true.
                    volVisMenu.setBaseRenderResolution(baseRenderResolution / resolutionScale);
                    redrawFullResolution = true;
                    prevResolutionScale = resolutionScale;
                } else {
                    prevResolutionScale = 1;
                    volVisMenu.setBaseRenderResolution(baseRenderResolution);
                    redrawFullResolution = false;
                }
                redrawUserInteraction = false;

                using clock = std::chrono::high_resolution_clock;
                const auto start = clock::now();
                optRenderer->render();
                const auto end = clock::now();
                renderTime = end - start;

                fullScreenTextureGL.update(optRenderer->frameBuffer(), volVisMenu.renderConfig().renderResolution);
            }

            // === Drawing the framebuffer to the screen and adding the wireframe. ===

            // Make the wireframe slightly larger than the volume to prevent z-fighting
            constexpr float wireframeMargin = 0.05f;
            const auto wireframeCubeSize = glm::vec3(optVolume->dims()) * (1.0f + wireframeMargin);
            const auto wireframeCubeOffset = -glm::vec3(optVolume->dims()) * wireframeMargin * 0.5f;
            constexpr glm::vec3 wireframeColor { 1.0f };

            // Draw on the left side of the screen next to the menu.
            const glm::ivec2 borders = ((windowSize - glm::ivec2(menuWidth, 0) - baseRenderResolution)) / 2;
            glViewport(borders.x, borders.y, GLsizei(baseRenderResolution.x * dpiScaling.x), GLsizei(baseRenderResolution.y * dpiScaling.y));

            // Enable depth testing and clear the color/depth buffers.
            glEnable(GL_DEPTH_TEST);
            glClearDepthf(1.0f);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Enable normal depth testing and draw an invisible (no color write) solid cube to the depth buffer.
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            surfaceCube.draw(trackballCamera, optVolume->dims());

            // Enable color writes and depth blending.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Draw the part of the wireframe that is behind the volume.
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_GREATER);
            wireframeCube.draw(trackballCamera, wireframeCubeSize, wireframeCubeOffset, wireframeColor);

            // Draw the CPU framebuffer on top of the GPU framebuffer.
            glDepthFunc(GL_ALWAYS);
            //  Assume that the renderer already multiplied the RGB channels by alpha.
            glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            fullScreenTextureGL.draw();

            // Finally, draw the part of the wireframe that is in front of the volume.
            glDepthFunc(GL_LEQUAL);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            wireframeCube.draw(trackballCamera, wireframeCubeSize, wireframeCubeOffset, wireframeColor);

            // Restore render state.
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);

            //wireframeCube.draw(trackballCamera, wireframeCubeSize, wireframeCubeOffset, wireframeColor);
        } else {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        // Close window by pressing the escape key.
        if (myWindow.isKeyPressed(GLFW_KEY_ESCAPE))
            break;

        volVisMenu.drawMenu(glm::ivec2(windowSize.x - menuWidth, 0), glm::ivec2(menuWidth, windowSize.y), renderTime);

        myWindow.swapBuffers();
    }

    return 0;
}
