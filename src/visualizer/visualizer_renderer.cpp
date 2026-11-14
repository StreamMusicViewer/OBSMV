#include "visualizer_renderer.hpp"
#include "../render/color_utils.hpp"
#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace obsmv {

static void set_effect_color(gs_effect_t* effect, uint32_t rgba) {
    gs_eparam_t* param = gs_effect_get_param_by_name(effect, "color");
    if (!param) return;
    struct vec4 v;
    vec4_from_rgba(&v, rgba);
    gs_effect_set_vec4(param, &v);
}

VisualizerRenderer::VisualizerRenderer() {}
VisualizerRenderer::~VisualizerRenderer() {}

void VisualizerRenderer::render(const std::vector<float>& left_bins,
                                const std::vector<float>& right_bins,
                                const VisualizerRenderConfig& cfg,
                                uint32_t dynamic_accent_color,
                                uint32_t dynamic_primary_color,
                                float origin_x, float origin_y,
                                float width, float height) {
    if (left_bins.empty() || width <= 0.0f || height <= 0.0f) return;

    // Start with user-configured colors
    uint32_t c_top = cfg.color_top;
    uint32_t c_bottom = cfg.color_bottom;

    if (cfg.dynamic_color) {
        // Photoshop-style color influence:
        // Top: vibrant accent color from the album art
        // Bottom: dominant primary base color from the album art
        float inf = std::clamp(cfg.dynamic_influence, 0.0f, 1.0f);
        c_top = lerp_color(cfg.color_top, dynamic_accent_color, inf);
        c_bottom = lerp_color(cfg.color_bottom, dynamic_primary_color, inf);
    }

    // Apply overall visualizer opacity slider
    if (cfg.opacity < 1.0f) {
        c_top = multiply_alpha(c_top, cfg.opacity);
        c_bottom = multiply_alpha(c_bottom, cfg.opacity);
    }

    gs_matrix_push();
    gs_matrix_translate3f(origin_x, origin_y, 0.0f);

    switch (cfg.style) {
        case VisualizerStyle::Bars:
            render_bars(left_bins, cfg, c_top, c_bottom, 0, 0, width, height);
            break;
        case VisualizerStyle::Mirrored:
            render_mirrored(left_bins, cfg, c_top, c_bottom, 0, 0, width, height);
            break;
        case VisualizerStyle::Wave:
            render_wave(left_bins, cfg, c_top, c_bottom, 0, 0, width, height);
            break;
        case VisualizerStyle::Circular:
            render_circular(left_bins, cfg, c_top, c_bottom, width * 0.5f, height * 0.5f);
            break;
    }

    gs_matrix_pop();
}

void VisualizerRenderer::render_bars(const std::vector<float>& bins,
                                    const VisualizerRenderConfig& cfg,
                                    uint32_t c_top, uint32_t c_bottom,
                                    float /*x*/, float /*y*/, float width, float height) {
    int count = std::min<int>(static_cast<int>(bins.size()), cfg.bar_count);
    if (count <= 0) return;

    float step = width / static_cast<float>(count);
    float bar_w = std::max(1.0f, step - cfg.bar_gap);

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    if (!solid) return;
    gs_technique_t* tech_colored = gs_effect_get_technique(solid, "SolidColored");
    if (!tech_colored) return;

    gs_blend_state_push();
    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    // Draw all bars in a single vertex buffer batch with vertical gradient
    size_t passes = gs_technique_begin(tech_colored);
    for (size_t p = 0; p < passes; ++p) {
        gs_technique_begin_pass(tech_colored, p);

        gs_render_start(true);
        for (int i = 0; i < count; ++i) {
            float val = std::clamp(bins[i], 0.0f, 1.0f);
            float bar_h = val * height;
            if (bar_h < 2.0f) bar_h = 2.0f; // Minimal baseline height

            float pos_x1 = i * step + (step - bar_w) * 0.5f;
            float pos_x2 = pos_x1 + bar_w;
            float pos_y_top = height - bar_h;
            float pos_y_bot = height;

            // Triangle 1
            gs_color(c_top);    gs_vertex2f(pos_x1, pos_y_top);
            gs_color(c_top);    gs_vertex2f(pos_x2, pos_y_top);
            gs_color(c_bottom); gs_vertex2f(pos_x1, pos_y_bot);

            // Triangle 2
            gs_color(c_top);    gs_vertex2f(pos_x2, pos_y_top);
            gs_color(c_bottom); gs_vertex2f(pos_x2, pos_y_bot);
            gs_color(c_bottom); gs_vertex2f(pos_x1, pos_y_bot);
        }
        gs_vertbuffer_t* vb = gs_render_save();
        if (vb) {
            gs_load_vertexbuffer(vb);
            gs_draw(GS_TRIS, 0, 0);
            gs_vertexbuffer_destroy(vb);
        }

        gs_technique_end_pass(tech_colored);
    }
    gs_technique_end(tech_colored);

    gs_blend_state_pop();
}

void VisualizerRenderer::render_mirrored(const std::vector<float>& bins,
                                        const VisualizerRenderConfig& cfg,
                                        uint32_t c_top, uint32_t c_bottom,
                                        float /*x*/, float /*y*/, float width, float height) {
    int count = std::min<int>(static_cast<int>(bins.size()), cfg.bar_count);
    if (count <= 0) return;

    float step = width / static_cast<float>(count);
    float bar_w = std::max(1.0f, step - cfg.bar_gap);
    float center_y = height * 0.5f;

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    if (!solid) return;
    gs_technique_t* tech_colored = gs_effect_get_technique(solid, "SolidColored");
    if (!tech_colored) return;

    gs_blend_state_push();
    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    size_t passes = gs_technique_begin(tech_colored);
    for (size_t p = 0; p < passes; ++p) {
        gs_technique_begin_pass(tech_colored, p);

        gs_render_start(true);
        for (int i = 0; i < count; ++i) {
            float val = std::clamp(bins[i], 0.0f, 1.0f);
            float half_h = val * (height * 0.48f);
            if (half_h < 1.0f) half_h = 1.0f;

            float pos_x1 = i * step + (step - bar_w) * 0.5f;
            float pos_x2 = pos_x1 + bar_w;
            float top_y = center_y - half_h;
            float bot_y = center_y + half_h;

            // Top half: c_top at tip, c_bottom at center
            gs_color(c_top);    gs_vertex2f(pos_x1, top_y);
            gs_color(c_top);    gs_vertex2f(pos_x2, top_y);
            gs_color(c_bottom); gs_vertex2f(pos_x1, center_y);

            gs_color(c_top);    gs_vertex2f(pos_x2, top_y);
            gs_color(c_bottom); gs_vertex2f(pos_x2, center_y);
            gs_color(c_bottom); gs_vertex2f(pos_x1, center_y);

            // Bottom half: c_bottom at center, c_top at bottom tip
            gs_color(c_bottom); gs_vertex2f(pos_x1, center_y);
            gs_color(c_bottom); gs_vertex2f(pos_x2, center_y);
            gs_color(c_top);    gs_vertex2f(pos_x1, bot_y);

            gs_color(c_bottom); gs_vertex2f(pos_x2, center_y);
            gs_color(c_top);    gs_vertex2f(pos_x2, bot_y);
            gs_color(c_top);    gs_vertex2f(pos_x1, bot_y);
        }
        gs_vertbuffer_t* vb = gs_render_save();
        if (vb) {
            gs_load_vertexbuffer(vb);
            gs_draw(GS_TRIS, 0, 0);
            gs_vertexbuffer_destroy(vb);
        }

        gs_technique_end_pass(tech_colored);
    }
    gs_technique_end(tech_colored);

    gs_blend_state_pop();
}

void VisualizerRenderer::render_wave(const std::vector<float>& bins,
                                    const VisualizerRenderConfig& cfg,
                                    uint32_t c_top, uint32_t c_bottom,
                                    float /*x*/, float /*y*/, float width, float height) {
    int count = static_cast<int>(bins.size());
    if (count < 2 || width <= 0.0f || height <= 0.0f) return;

    // The base of the wave is at height (100%), so it sits seamlessly flat on the bottom
    float base_y = height;

    struct Point { float x; float y; };
    std::vector<Point> pts(count);
    float step = width / static_cast<float>(count - 1);
    for (int i = 0; i < count; ++i) {
        float val = std::clamp(bins[i], 0.0f, 1.0f);
        float py = base_y - (val * (height - 2.0f));
        pts[i] = { i * step, py };
    }
    pts.front().x = 0.0f;
    pts.back().x = width;

    // Catmull-Rom cubic spline interpolation for silky-smooth, fluid rounded curves
    constexpr int subdivisions = 12;
    std::vector<Point> spline_pts;
    spline_pts.reserve((count - 1) * subdivisions + 1);

    for (int i = 0; i < count - 1; ++i) {
        Point p0 = (i > 0) ? pts[i - 1] : pts[i];
        Point p1 = pts[i];
        Point p2 = pts[i + 1];
        Point p3 = (i + 2 < count) ? pts[i + 2] : pts[i + 1];

        for (int s = 0; s < subdivisions; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(subdivisions);
            float t2 = t * t;
            float t3 = t2 * t;

            float sx = 0.5f * ((2.0f * p1.x) +
                               (-p0.x + p2.x) * t +
                               (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                               (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);

            float sy = 0.5f * ((2.0f * p1.y) +
                               (-p0.y + p2.y) * t +
                               (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                               (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);

            sx = std::clamp(sx, 0.0f, width);
            sy = std::clamp(sy, 0.0f, base_y);
            spline_pts.push_back({ sx, sy });
        }
    }
    spline_pts.push_back(pts.back());

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    if (!solid) return;
    gs_technique_t* tech_colored = gs_effect_get_technique(solid, "SolidColored");
    if (!tech_colored) return;

    gs_blend_state_push();
    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    // 1. Draw Wave Fill with Vertical Gradient from top crest to base
    size_t passes = gs_technique_begin(tech_colored);
    for (size_t p = 0; p < passes; ++p) {
        gs_technique_begin_pass(tech_colored, p);

        gs_render_start(true);
        for (const auto& pt : spline_pts) {
            gs_color(c_top);
            gs_vertex2f(pt.x, pt.y);

            gs_color(c_bottom);
            gs_vertex2f(pt.x, base_y);
        }
        gs_vertbuffer_t* vb = gs_render_save();
        if (vb) {
            gs_load_vertexbuffer(vb);
            gs_draw(GS_TRISTRIP, 0, 0);
            gs_vertexbuffer_destroy(vb);
        }

        gs_technique_end_pass(tech_colored);
    }
    gs_technique_end(tech_colored);

    // 2. Glowing Crest Stroke Line along the spline curve
    if (cfg.wave_crest && spline_pts.size() >= 2) {
        uint32_t c_crest = multiply_alpha(c_top, 1.0f);

        passes = gs_technique_begin(tech_colored);
        for (size_t p = 0; p < passes; ++p) {
            gs_technique_begin_pass(tech_colored, p);

            gs_render_start(true);
            for (const auto& pt : spline_pts) {
                gs_color(c_crest);
                gs_vertex2f(pt.x, pt.y - 1.25f);
                gs_color(c_crest);
                gs_vertex2f(pt.x, pt.y + 1.25f);
            }
            gs_vertbuffer_t* vb = gs_render_save();
            if (vb) {
                gs_load_vertexbuffer(vb);
                gs_draw(GS_TRISTRIP, 0, 0);
                gs_vertexbuffer_destroy(vb);
            }

            gs_technique_end_pass(tech_colored);
        }
        gs_technique_end(tech_colored);
    }

    gs_blend_state_pop();
}

void VisualizerRenderer::render_circular(const std::vector<float>& bins,
                                        const VisualizerRenderConfig& cfg,
                                        uint32_t c_top, uint32_t c_bottom,
                                        float cx, float cy) {
    int count = std::min<int>(static_cast<int>(bins.size()), cfg.bar_count);
    if (count <= 0) return;

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    if (!solid) return;
    gs_technique_t* tech_colored = gs_effect_get_technique(solid, "SolidColored");
    if (!tech_colored) return;

    gs_blend_state_push();
    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    float angle_step = (2.0f * static_cast<float>(M_PI)) / count;
    float base_radius = cfg.circular_radius;
    float bar_w = std::max(2.0f, static_cast<float>(cfg.bar_width));

    size_t passes = gs_technique_begin(tech_colored);
    for (size_t p = 0; p < passes; ++p) {
        gs_technique_begin_pass(tech_colored, p);

        for (int i = 0; i < count; ++i) {
            float val = std::clamp(bins[i], 0.0f, 1.0f);
            float bar_h = val * static_cast<float>(cfg.max_height);
            if (bar_h < 2.0f) bar_h = 2.0f;

            float angle = i * angle_step + cfg.circular_rotation;

            gs_matrix_push();
            gs_matrix_translate3f(cx, cy, 0.0f);
            gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, (angle * 180.0f / static_cast<float>(M_PI)) - 90.0f);
            gs_matrix_translate3f(-bar_w * 0.5f, base_radius, 0.0f);

            gs_render_start(true);
            gs_color(c_bottom); gs_vertex2f(0.0f, 0.0f);
            gs_color(c_bottom); gs_vertex2f(bar_w, 0.0f);
            gs_color(c_top);    gs_vertex2f(0.0f, bar_h);

            gs_color(c_bottom); gs_vertex2f(bar_w, 0.0f);
            gs_color(c_top);    gs_vertex2f(bar_w, bar_h);
            gs_color(c_top);    gs_vertex2f(0.0f, bar_h);
            gs_vertbuffer_t* vb = gs_render_save();
            if (vb) {
                gs_load_vertexbuffer(vb);
                gs_draw(GS_TRIS, 0, 0);
                gs_vertexbuffer_destroy(vb);
            }

            gs_matrix_pop();
        }

        gs_technique_end_pass(tech_colored);
    }
    gs_technique_end(tech_colored);

    gs_blend_state_pop();
}

} // namespace obsmv
