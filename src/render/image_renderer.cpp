#include "image_renderer.hpp"
#include "color_utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"

#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <algorithm>
#include <cmath>

namespace obsmv {

ImageRenderer::ImageRenderer() {
    accent_color_ = make_obs_color(170, 0, 255, 255);
    primary_color_ = make_obs_color(20, 20, 24, 255);
}

ImageRenderer::~ImageRenderer() {
    free_texture();
}

void ImageRenderer::free_texture() {
    if (texture_) {
        obs_enter_graphics();
        gs_texture_destroy(texture_);
        obs_leave_graphics();
        texture_ = nullptr;
    }
    width_ = 0;
    height_ = 0;
    current_hash_ = 0;
}

void ImageRenderer::compute_palette(const uint8_t* rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0) {
        accent_color_ = make_obs_color(170, 0, 255, 255);
        primary_color_ = make_obs_color(20, 20, 24, 255);
        return;
    }

    // 1. Accent color: most vibrant, saturated midtone pixel
    uint64_t best_accent_score = 0;
    uint8_t acc_r = 170, acc_g = 0, acc_b = 255;

    // 2. Primary color: dominant color bucket (8x8x8 = 512 buckets)
    struct Bucket {
        uint32_t count = 0;
        uint64_t sum_r = 0;
        uint64_t sum_g = 0;
        uint64_t sum_b = 0;
    };
    std::vector<Bucket> buckets(512);

    int total_pixels = w * h;
    int step = std::max(1, total_pixels / 2500);

    for (int i = 0; i < total_pixels; i += step) {
        int idx = i * 4;
        uint8_t r = rgba[idx];
        uint8_t g = rgba[idx + 1];
        uint8_t b = rgba[idx + 2];
        uint8_t a = rgba[idx + 3];
        if (a < 128) continue;

        // Saturation & brightness
        uint8_t max_c = std::max({r, g, b});
        uint8_t min_c = std::min({r, g, b});
        int saturation = max_c - min_c;
        int brightness = (static_cast<int>(r) + g + b) / 3;

        if (saturation >= 12 || (brightness >= 25 && brightness <= 235)) {
            int tone_factor = 255 - std::abs(brightness - 128);
            uint64_t score = static_cast<uint64_t>(saturation) * saturation * tone_factor;
            if (score > best_accent_score) {
                best_accent_score = score;
                acc_r = r;
                acc_g = g;
                acc_b = b;
            }
        }

        // Primary color histogram bucket (3 bits per channel)
        int b_r = r >> 5;
        int b_g = g >> 5;
        int b_b = b >> 5;
        int b_idx = (b_r << 6) | (b_g << 3) | b_b;
        buckets[b_idx].count++;
        buckets[b_idx].sum_r += r;
        buckets[b_idx].sum_g += g;
        buckets[b_idx].sum_b += b;
    }

    if (best_accent_score == 0) {
        acc_r = 170; acc_g = 0; acc_b = 255;
    }

    // Find the most frequent color bucket
    uint32_t max_count = 0;
    int best_bucket = -1;
    for (int b = 0; b < 512; ++b) {
        if (buckets[b].count > max_count) {
            max_count = buckets[b].count;
            best_bucket = b;
        }
    }

    uint8_t prim_r = 20, prim_g = 20, prim_b = 24;
    if (best_bucket >= 0 && buckets[best_bucket].count > 0) {
        prim_r = static_cast<uint8_t>(buckets[best_bucket].sum_r / buckets[best_bucket].count);
        prim_g = static_cast<uint8_t>(buckets[best_bucket].sum_g / buckets[best_bucket].count);
        prim_b = static_cast<uint8_t>(buckets[best_bucket].sum_b / buckets[best_bucket].count);
    }

    accent_color_ = make_obs_color(acc_r, acc_g, acc_b, 255);
    primary_color_ = make_obs_color(prim_r, prim_g, prim_b, 255);
}

bool ImageRenderer::is_dark_artwork() const {
    uint8_t r, g, b, a;
    unpack_obs_color(primary_color_, r, g, b, a);
    float lum = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
    return lum < 0.52f;
}

bool ImageRenderer::update_image(const std::vector<uint8_t>& image_bytes, uint64_t hash) {
    if (image_bytes.empty()) {
        free_texture();
        accent_color_ = make_obs_color(170, 0, 255, 255);
        primary_color_ = make_obs_color(20, 20, 24, 255);
        return false;
    }

    if (texture_ && hash == current_hash_ && hash != 0) {
        return true;
    }

    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(image_bytes.data(),
                                                  static_cast<int>(image_bytes.size()),
                                                  &w, &h, &channels, 4);
    if (!pixels) {
        return false;
    }

    free_texture();

    compute_palette(pixels, w, h);

    obs_enter_graphics();
    const uint8_t* raw_ptr = pixels;
    texture_ = gs_texture_create(w, h, GS_RGBA, 1, &raw_ptr, 0);
    obs_leave_graphics();

    width_ = static_cast<uint32_t>(w);
    height_ = static_cast<uint32_t>(h);
    current_hash_ = hash;

    stbi_image_free(pixels);
    return texture_ != nullptr;
}

void ImageRenderer::render(float x, float y, float size, float /*corner_radius*/, float rotation_degrees) {
    if (!texture_ || size <= 0.0f) return;

    gs_effect_t* effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    if (!effect) return;

    gs_eparam_t* image_param = gs_effect_get_param_by_name(effect, "image");
    gs_technique_t* tech = gs_effect_get_technique(effect, "Draw");
    if (!image_param || !tech) return;

    gs_blend_state_push();
    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    gs_effect_set_texture(image_param, texture_);

    gs_matrix_push();
    gs_matrix_translate3f(x + size * 0.5f, y + size * 0.5f, 0.0f);

    if (rotation_degrees != 0.0f) {
        gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, rotation_degrees);
    }

    gs_matrix_translate3f(-size * 0.5f, -size * 0.5f, 0.0f);

    size_t passes = gs_technique_begin(tech);
    for (size_t i = 0; i < passes; ++i) {
        gs_technique_begin_pass(tech, i);
        gs_draw_sprite(texture_, 0, static_cast<uint32_t>(size), static_cast<uint32_t>(size));
        gs_technique_end_pass(tech);
    }
    gs_technique_end(tech);

    gs_matrix_pop();
    gs_blend_state_pop();
}

} // namespace obsmv
