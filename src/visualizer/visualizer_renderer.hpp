#pragma once

#include "fft_processor.hpp"
#include <obs-module.h>
#include <graphics/graphics.h>
#include <vector>

namespace obsmv {

enum class VisualizerStyle {
    Bars = 0,
    Mirrored,
    Wave,
    Circular
};

struct VisualizerRenderConfig {
    VisualizerStyle style = VisualizerStyle::Bars;
    int bar_count = 32;
    int bar_width = 8;
    int bar_gap = 4;
    int max_height = 80;
    int corner_radius = 4;
    uint32_t color_top = 0xFFFF00AA;    // RGBA
    uint32_t color_bottom = 0xFF7F0055; // RGBA
    bool use_gradient = true;
    bool dynamic_color = true;
    float dynamic_influence = 1.0f; // 0.0 (manual color) to 1.0 (100% album cover color)
    float opacity = 1.0f;           // Overall visualizer opacity (0.0 to 1.0)
    bool wave_crest = true;         // Luminous top crest stroke line
    float wave_smoothness = 0.75f;  // Spline smoothing factor
    float circular_radius = 50.0f;
    float circular_rotation = 0.0f;
};

class VisualizerRenderer {
public:
    VisualizerRenderer();
    ~VisualizerRenderer();

    void render(const std::vector<float>& left_bins,
                const std::vector<float>& right_bins,
                const VisualizerRenderConfig& cfg,
                uint32_t dynamic_accent_color,
                uint32_t dynamic_primary_color,
                float origin_x, float origin_y,
                float width, float height);

private:
    void render_bars(const std::vector<float>& bins,
                     const VisualizerRenderConfig& cfg,
                     uint32_t c_top, uint32_t c_bottom,
                     float x, float y, float w, float h);

    void render_mirrored(const std::vector<float>& bins,
                         const VisualizerRenderConfig& cfg,
                         uint32_t c_top, uint32_t c_bottom,
                         float x, float y, float w, float h);

    void render_wave(const std::vector<float>& bins,
                     const VisualizerRenderConfig& cfg,
                     uint32_t c_top, uint32_t c_bottom,
                     float x, float y, float w, float h);

    void render_circular(const std::vector<float>& bins,
                         const VisualizerRenderConfig& cfg,
                         uint32_t c_top, uint32_t c_bottom,
                         float cx, float cy);
};

} // namespace obsmv
