#include "ui/transfer_func.h"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <gsl/span>
#include <imgui.h>
#include <iostream>

static GLuint createTexture();
static std::vector<glm::vec4> createHistogramImage(gsl::span<const int> data, float opacity);
static ImVec2 glmToIm(const glm::vec2& v);
static glm::vec2 ImToGlm(const ImVec2& v);

// Radius of the points in the histogram image.
static constexpr float pointRadius = 8.0f;
static constexpr glm::ivec2 widgetSize { 475, 300 };
static constexpr float histogramOpacity = 0.3f;
static constexpr size_t sentinel = static_cast<size_t>(-1);

namespace ui {

TransferFunctionWidget::TransferFunctionWidget(const volume::Volume& volume)
    : m_colorMap(256)
    , m_minValue(volume.minimum())
    , m_maxValue(volume.maximum())
    , m_interactingPoint(sentinel)
    , m_selectedPoint(sentinel)
    , m_histogramImg(createTexture())
    , m_colorMapImg(createTexture())
{
    m_tfPoints.push_back(TFPoint { glm::vec2(0.0f), glm::vec3(0.0f) });
    m_tfPoints.push_back(TFPoint { glm::vec2(0.7f, 0.03f), glm::vec3(0.7f) });
    m_tfPoints.push_back(TFPoint { glm::vec2(1.0f), glm::vec3(1.0f) });

    const auto histogram = volume.histogram();
    const auto imgData = createHistogramImage(histogram, histogramOpacity);

    glBindTexture(GL_TEXTURE_2D, m_histogramImg);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GLsizei(histogram.size()), GLsizei(widgetSize.y), 0, GL_RGBA, GL_FLOAT, imgData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    updateColormap();
}

void TransferFunctionWidget::updateRenderConfig(render::RenderConfig& renderConfig) const
{
    assert(m_colorMap.size() == renderConfig.tfColorMap.size());
    std::copy(std::begin(m_colorMap), std::end(m_colorMap), std::begin(renderConfig.tfColorMap));
    // Color map ranges from 0 to volume.maximum(). See volume.histogram() for details...
    renderConfig.tfColorMapIndexStart = 0;
    renderConfig.tfColorMapIndexRange = m_maxValue;
}

// Draw the widget and handle interactions.
void TransferFunctionWidget::draw()
{
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::Text("Transfer Function");
    ImGui::TextWrapped("Left click to add a point, right click remove. Left click + drag to move points.");

    // Histogram image is positioned to the right of the content region.
    const glm::vec2 canvasSize { widgetSize.x, widgetSize.y - 20 };
    glm::vec2 canvasPos = ImToGlm(ImGui::GetCursorScreenPos()); // this is the imgui draw cursor, not mouse cursor
    const float xOffset = (ImToGlm(ImGui::GetContentRegionAvail()).x - canvasSize.x);
    canvasPos.x += xOffset; // center widget

    // Draw side text (imgui cannot center-align text so we have to do it ourselves).
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursorPos.x + 35, cursorPos.y + canvasSize.y / 2 - ImGui::GetFontSize() - 20));
    //ImGui::Text("");
    ImGui::Separator();
    ImGui::SetCursorPos(ImVec2(cursorPos.x + 5, cursorPos.y + canvasSize.y / 2 - 20));
    ImGui::Text("Opacity");
    ImGui::SetCursorPos(cursorPos);

    // Draw box and histogram image.
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(glmToIm(canvasPos), glmToIm(canvasPos + canvasSize));
    drawList->AddRect(glmToIm(canvasPos), glmToIm(canvasPos + canvasSize), ImColor(180, 180, 180, 255));

    cursorPos = ImVec2(ImGui::GetCursorPosX() + xOffset, ImGui::GetCursorPosY());

    // Draw histogram image that we uploaded to the GPU using OpenGL.
    ImGui::SetCursorPos(cursorPos);

    // NOTE(Mathijs): reinterpret casting the pointer is undefined behavior according to the standard.
    // Use memcpy for now and move to std::bit_cast (C++20) when more compilers support it.
    //
    // From the standard:
    // When it is needed to interpret the bytes of an object as a value of a different type, std::memcpy or std::bit_cast (since C++20)can be used:
    // https://en.cppreference.com/w/cpp/language/reinterpret_cast
    ImTextureID imguiTexture;
    std::memcpy(&imguiTexture, &m_histogramImg, sizeof(m_histogramImg));
    ImGui::Image(imguiTexture, glmToIm(canvasSize - glm::vec2(1)));

    // Detect and handle mouse interaction.
    if (!io.MouseDown[0] && !io.MouseDown[1]) {
        m_interactingPoint = sentinel;
    }

    // Place an invisible button on top of the histogram. IsItemHovering returns whether the cursor is
    // hovering over the last added item which in this case is the invisble button. This way we can
    // easily detect whether the cursor is inside the histogram image.
    ImGui::SetCursorPos(cursorPos);
    ImGui::InvisibleButton("tfn_canvas", glmToIm(canvasSize));

    // Mouse position within the histogram image.
    const ImVec2 bbMin = ImGui::GetItemRectMin();
    const ImVec2 bbMax = ImGui::GetItemRectMax();
    const glm::vec2 clippedMousePos {
        std::min(std::max(io.MousePos.x, bbMin.x), bbMax.x),
        std::min(std::max(io.MousePos.y, bbMin.y), bbMax.y)
    };

    const glm::vec2 viewScale(canvasSize.x, -canvasSize.y);
    const glm::vec2 viewOffset(canvasPos.x, canvasPos.y + canvasSize.y);
    if (ImGui::IsItemHovered() && (io.MouseDown[0] || io.MouseDown[1])) {
        const glm::vec2 mousePos = glm::clamp((clippedMousePos - viewOffset) / viewScale, 0.0f, 1.0f);

        // No point is currently selected. Check if the user clicked on a point.
        if (m_interactingPoint == sentinel) {
            for (size_t i = 0; i < m_tfPoints.size(); i++) {
                const glm::vec2 ptPos = m_tfPoints[i].pos * viewScale + viewOffset;
                const glm::vec2 d = ptPos - clippedMousePos;
                const float dSqr = glm::dot(d, d);
                if (dSqr < pointRadius * pointRadius) {
                    m_interactingPoint = i;
                    break;
                }
            }
        }

        if (io.MouseDown[0]) {
            // Left Mouse Button => move or add point.
            if (m_interactingPoint != sentinel) {
                m_selectedPoint = m_interactingPoint;
                auto& selectedPoint = m_tfPoints[m_interactingPoint];
                selectedPoint.pos = mousePos;

                // Clamp the horizontal movement of the moved point.
                if (m_interactingPoint == 0) {
                    selectedPoint.pos.x = 0.f;
                } else if (m_interactingPoint == m_tfPoints.size() - 1) {
                    selectedPoint.pos.x = 1;
                } else {
                    const auto& prevPoint = m_tfPoints[m_interactingPoint - 1];
                    const auto& nextPoint = m_tfPoints[m_interactingPoint + 1];
                    selectedPoint.pos.x = std::clamp(selectedPoint.pos.x, prevPoint.pos.x, nextPoint.pos.x);
                }
            } else {
                // If no point was clicked, insert a new point.
                insertTFPoint(mousePos);
            }
        } else if (io.MouseDown[1]) {
            // Right Mouse Button => remove point.
            if (m_interactingPoint != sentinel) {
                if (m_interactingPoint != 0 && m_interactingPoint != m_tfPoints.size() - 1) {
                    m_tfPoints.erase(m_tfPoints.begin() + static_cast<int>(m_interactingPoint));
                    m_interactingPoint = sentinel;
                    m_selectedPoint = sentinel;
                }
            }
        }

        updateColormap();
    }

    // Draw the alpha control points and connecting polyline.
    for (size_t i = 0; i < m_tfPoints.size() - 1; i++) {
        const ImVec2 ptPos0 = glmToIm(m_tfPoints[i].pos * viewScale + viewOffset);
        const ImVec2 ptPos1 = glmToIm(m_tfPoints[i + 1].pos * viewScale + viewOffset);
        drawList->AddLine(ptPos0, ptPos1, 0xFFFFFFFF);
    }
    for (size_t i = 0; i < m_tfPoints.size(); i++) {
        const ImVec2 ptPos = glmToIm(m_tfPoints[i].pos * viewScale + viewOffset);
        drawList->AddCircleFilled(ptPos, pointRadius, (i == m_selectedPoint) ? 0xFFAAFFFF : 0xFFFFFFFF);
        drawList->AddCircleFilled(ptPos, pointRadius * 0.6f, ImColor(m_tfPoints[i].color.r, m_tfPoints[i].color.g, m_tfPoints[i].color.b, 1.0f));
    }

    drawList->PopClipRect();

    // Draw colormap.
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
    std::memcpy(&imguiTexture, &m_colorMapImg, sizeof(m_colorMapImg));
    ImGui::Image(imguiTexture, ImVec2(canvasSize.x, 16));

    // Bottom text
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset + canvasSize.x / 2 - 40);
    ImGui::Text("Voxel Value");

    if (m_selectedPoint != sentinel) {
        ImGui::NewLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset / 2);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.4f);
        ImGui::ColorPicker3("Color", glm::value_ptr(m_tfPoints[m_selectedPoint].color));
        if (ImGui::IsItemActive())
            updateColormap();
    }
}

// (Re)compute the colormap color array
void TransferFunctionWidget::updateColormap()
{
    // Update the color map texture.
    auto left = std::begin(m_tfPoints);
    auto right = ++std::begin(m_tfPoints);
    for (size_t x = 0; x < m_colorMap.size(); x++) {
        if (static_cast<float>(x) > right->pos.x * static_cast<float>(m_colorMap.size())) {
            ++left;
            ++right;
        }

        m_colorMap[x] = glm::mix(TFPtoRGBA(*left), TFPtoRGBA(*right), (static_cast<float>(x) / static_cast<float>(m_colorMap.size()) - left->pos.x) / (right->pos.x - left->pos.x));
    }

    // Upload it to the GPU.
    glBindTexture(GL_TEXTURE_2D, m_colorMapImg);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, GLsizei(m_colorMap.size()), 1, 0, GL_RGBA, GL_FLOAT, m_colorMap.data());
}

void TransferFunctionWidget::insertTFPoint(const glm::vec2& pos)
{
    const auto compare = [](const TFPoint& lhs, const TFPoint& rhs) { return lhs.pos.x < rhs.pos.x; };
    const auto rightNeighbour = std::upper_bound(std::begin(m_tfPoints), std::end(m_tfPoints), TFPoint { pos, glm::vec3(1) }, compare);
    auto leftNeighbour = rightNeighbour;
    --leftNeighbour;

    const float d = ((rightNeighbour->pos.x - leftNeighbour->pos.x) - pos.x) / (rightNeighbour->pos.x - leftNeighbour->pos.x);
    const glm::vec3 color = glm::mix(leftNeighbour->color, rightNeighbour->color, d);
    m_interactingPoint = static_cast<size_t>(std::distance(std::begin(m_tfPoints), rightNeighbour)); // Call this before inserting (invalidated iterators).
    m_tfPoints.insert(rightNeighbour, TFPoint { pos, color });
}

glm::vec4 TransferFunctionWidget::TFPtoRGBA(const TFPoint& p)
{
    // Extract the rgb and alpha values from a corresponding TFPoint in the widget.
    return glm::vec4(p.color, p.pos.y);
}

}

GLuint createTexture()
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

// Compute a histogram texture from the histogram vector
static std::vector<glm::vec4> createHistogramImage(gsl::span<const int> data, float opacity)
{
    const int maxVal = *std::max_element(std::begin(data), std::end(data));
    const glm::uvec2 res { data.size(), widgetSize.y };

    const float scale = float(widgetSize.y) / (float(maxVal) * 1.1f);
    std::vector<glm::vec4> imgData(static_cast<size_t>(res.x * res.y));
    for (unsigned x = 0; x < res.x; x++) {
        for (unsigned y = 0; y < res.y; y++) {
            const size_t index = static_cast<size_t>(x + y * res.x);
            imgData[index] = (static_cast<float>(res.y - y) < static_cast<float>(data[x]) * scale) ? glm::vec4(1.f, 1.f, 1.f, opacity) : glm::vec4(0.f);
        }
    }
    return imgData;
}

// Vector conversion functions for glm - Imgui interaction
static ImVec2 glmToIm(const glm::vec2& v)
{
    return ImVec2(v.x, v.y);
}

static glm::vec2 ImToGlm(const ImVec2& v)
{
    return glm::vec2(v.x, v.y);
}
