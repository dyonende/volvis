#pragma once
#include "render/render_config.h"
#include "volume/volume.h"
#include <GL/glew.h> // Include before glfw3
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace ui {

class TransferFunctionWidget {
public:
    TransferFunctionWidget(const volume::Volume& volume);

    void draw();
    void updateRenderConfig(render::RenderConfig& renderConfig) const;

private:
    void updateColormap();
    void insertTFPoint(const glm::vec2& pos);

    struct TFPoint;
    glm::vec4 TFPtoRGBA(const TFPoint& p);

private:
    struct TFPoint {
        glm::vec2 pos; // x: scalar value , y: opacity
        glm::vec3 color;
    };

    std::vector<TFPoint> m_tfPoints;
    std::vector<glm::vec4> m_colorMap;
    float m_minValue, m_maxValue;

    size_t m_interactingPoint; // Point currently being dragged around.
    size_t m_selectedPoint; // Point that is selected (for which the color picker is shown).
    GLuint m_histogramImg;
    GLuint m_colorMapImg;
};
}
