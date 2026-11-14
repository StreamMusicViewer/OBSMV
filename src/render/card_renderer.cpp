#include "card_renderer.hpp"
#include "color_utils.hpp"
#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <cstdio>
#include <filesystem>
#include <algorithm>

namespace obsmv {

static void set_effect_color(gs_effect_t* effect, uint32_t rgba) {
    gs_eparam_t* param = gs_effect_get_param_by_name(effect, "color");
    if (!param) return;
    struct vec4 v;
    vec4_from_rgba(&v, rgba);
    gs_effect_set_vec4(param, &v);
}

CardRenderer::CardRenderer() {
    // 1. Try OBS module data path
    char* module_font = obs_module_file("fonts/Inter.ttf");
    if (module_font) {
        if (text_renderer_.load_font_file(module_font)) {
            bfree(module_font);
            return;
        }
        bfree(module_font);
    }

    // 2. Fallbacks: system fonts & relative paths
    std::filesystem::path font_candidates[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "data/fonts/Inter.ttf",
        "../data/fonts/Inter.ttf",
        "data/obs-plugins/obsmv/fonts/Inter.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc"
    };

    for (const auto& path : font_candidates) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            if (text_renderer_.load_font_file(path.string())) {
                break;
            }
        }
    }
}

CardRenderer::~CardRenderer() {
    text_renderer_.free_text_texture(title_tex_);
    text_renderer_.free_text_texture(artist_tex_);
    text_renderer_.free_text_texture(album_tex_);
    text_renderer_.free_text_texture(time_tex_);
}

void CardRenderer::configure(const CardConfig& cfg) {
    bool font_changed = (cfg.font_face != config_.font_face || cfg.font_path != config_.font_path);
    config_ = cfg;

    if (font_changed) {
        bool loaded = false;
        // If user specified custom file path, try that first
        if (!config_.font_path.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(config_.font_path, ec)) {
                loaded = text_renderer_.load_font_file(config_.font_path);
            }
        }
        // Otherwise, load system font by face name (from PC font selector)
        if (!loaded && !config_.font_face.empty()) {
            loaded = text_renderer_.load_system_font(config_.font_face);
        }
    }
}

std::string CardRenderer::format_string(const std::string& pattern, const SongInfo& song) {
    std::string res = pattern;
    auto replace_all = [&](const std::string& key, const std::string& val) {
        size_t pos = 0;
        while ((pos = res.find(key, pos)) != std::string::npos) {
            res.replace(pos, key.length(), val);
            pos += val.length();
        }
    };
    replace_all("{title}", song.title.empty() ? "No track playing" : song.title);
    replace_all("{artist}", song.artist);
    replace_all("{album}", song.album);
    replace_all("{player}", song.player);
    replace_all("$song", song.title.empty() ? "No track playing" : song.title);
    replace_all("$artist", song.artist);
    replace_all("$album", song.album);
    return res;
}

std::string CardRenderer::format_time(int64_t ms) {
    if (ms <= 0) return "0:00";
    int total_sec = static_cast<int>(ms / 1000);
    int m = total_sec / 60;
    int s = total_sec % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

void CardRenderer::draw_solid_rect(float x, float y, float w, float h, uint32_t rgba) {
    if (w < 1.0f || h < 1.0f) return;

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    if (!solid) return;
    gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");
    if (!tech) return;

    gs_blend_state_push();
    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    set_effect_color(solid, rgba);

    gs_matrix_push();
    gs_matrix_translate3f(x, y, 0.0f);

    size_t passes = gs_technique_begin(tech);
    for (size_t i = 0; i < passes; ++i) {
        gs_technique_begin_pass(tech, i);
        gs_draw_sprite(nullptr, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        gs_technique_end_pass(tech);
    }
    gs_technique_end(tech);

    gs_matrix_pop();
    gs_blend_state_pop();
}

void CardRenderer::update(const SongInfo& song, float dt_seconds) {
    current_song_ = song;

    // Update Cover
    image_renderer_.update_image(song.cover_data, song.cover_hash);

    // Update Vinyl Spin
    if (config_.spin_art && song.is_playing) {
        spin_angle_ += dt_seconds * 45.0f;
        if (spin_angle_ >= 360.0f) spin_angle_ -= 360.0f;
    }

    // Determine Theme (Dark vs Light) for optimal text contrast
    bool is_dark = true;
    if (config_.theme_mode == CardThemeMode::Light) {
        is_dark = false;
    } else if (config_.theme_mode == CardThemeMode::Auto) {
        is_dark = image_renderer_.is_dark_artwork();
    }

    uint32_t active_title = config_.title_color;
    uint32_t active_artist = config_.artist_color;
    uint32_t active_album = config_.album_color;
    uint32_t active_time = config_.time_color;

    if (config_.dynamic_color) {
        uint32_t dyn_title, dyn_artist, dyn_album, dyn_time;
        if (is_dark) {
            // Crisp high-contrast white & light grey for dark background
            dyn_title = make_obs_color(255, 255, 255, 255);
            dyn_artist = make_obs_color(210, 210, 215, 230);
            dyn_album = make_obs_color(165, 165, 175, 200);
            dyn_time = make_obs_color(180, 180, 185, 220);
        } else {
            // Crisp high-contrast deep charcoal & slate for light background
            dyn_title = make_obs_color(20, 20, 24, 255);
            dyn_artist = make_obs_color(70, 70, 80, 230);
            dyn_album = make_obs_color(105, 105, 118, 200);
            dyn_time = make_obs_color(85, 85, 95, 220);
        }

        float inf = std::clamp(config_.dynamic_influence, 0.0f, 1.0f);
        active_title = lerp_color(config_.title_color, dyn_title, inf);
        active_artist = lerp_color(config_.artist_color, dyn_artist, inf);
        active_album = lerp_color(config_.album_color, dyn_album, inf);
        active_time = lerp_color(config_.time_color, dyn_time, inf);
    }

    // Determine available text width
    float text_x = config_.padding + (config_.art_size + config_.padding);
    uint32_t max_text_w = static_cast<uint32_t>(std::max(50.0f, config_.width - text_x - config_.padding));

    std::string title_str = format_string(config_.title_format, song);
    std::string artist_str = format_string(config_.artist_format, song);
    std::string album_str = config_.show_album ? format_string(config_.album_format, song) : "";

    text_renderer_.update_text_texture(title_tex_, title_str, config_.title_size, active_title, max_text_w);
    text_renderer_.update_text_texture(artist_tex_, artist_str, config_.artist_size, active_artist, max_text_w);
    if (!album_str.empty()) {
        text_renderer_.update_text_texture(album_tex_, album_str, config_.album_size, active_album, max_text_w);
    } else {
        text_renderer_.free_text_texture(album_tex_);
    }

    // Progress time string
    if (config_.show_progress && config_.show_time && song.timeline.duration_ms > 0) {
        std::string time_str = format_time(song.timeline.position_ms) + " / " + format_time(song.timeline.duration_ms);
        text_renderer_.update_text_texture(time_tex_, time_str, config_.time_size, active_time);
    } else {
        text_renderer_.free_text_texture(time_tex_);
    }
}

void CardRenderer::render(float origin_x, float origin_y) {
    float w = static_cast<float>(config_.width);
    float h = static_cast<float>(config_.height);

    gs_matrix_push();
    gs_matrix_translate3f(origin_x, origin_y, 0.0f);

    bool is_dark = true;
    if (config_.theme_mode == CardThemeMode::Light) {
        is_dark = false;
    } else if (config_.theme_mode == CardThemeMode::Auto) {
        is_dark = image_renderer_.is_dark_artwork();
    }

    uint32_t active_bg = config_.bg_color;
    uint32_t active_border = config_.border_color;
    uint32_t active_prog = config_.progress_color;
    uint32_t active_prog_bg = config_.progress_bg;

    if (config_.dynamic_color) {
        uint8_t acc_r, acc_g, acc_b, acc_a;
        unpack_obs_color(image_renderer_.get_accent_color(), acc_r, acc_g, acc_b, acc_a);

        uint8_t prim_r, prim_g, prim_b, prim_a;
        unpack_obs_color(image_renderer_.get_primary_color(), prim_r, prim_g, prim_b, prim_a);

        uint32_t dyn_bg, dyn_border, dyn_prog, dyn_prog_bg;

        if (is_dark) {
            // Sleek dark glass tinted with primary & accent
            uint8_t bg_r = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(prim_r * 0.15f + acc_r * 0.05f) + 12, 10, 42));
            uint8_t bg_g = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(prim_g * 0.15f + acc_g * 0.05f) + 12, 10, 42));
            uint8_t bg_b = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(prim_b * 0.15f + acc_b * 0.05f) + 16, 12, 48));
            dyn_bg = make_obs_color(bg_r, bg_g, bg_b, 230);
            dyn_border = make_obs_color(acc_r, acc_g, acc_b, 140);
            dyn_prog = make_obs_color(acc_r, acc_g, acc_b, 255);
            dyn_prog_bg = make_obs_color(bg_r + 20, bg_g + 20, bg_b + 25, 200);
        } else {
            // Light frosted glass tinted with primary & accent
            uint8_t bg_r = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(prim_r * 0.20f) + 205, 200, 245));
            uint8_t bg_g = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(prim_g * 0.20f) + 205, 200, 245));
            uint8_t bg_b = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(prim_b * 0.20f) + 208, 200, 248));
            dyn_bg = make_obs_color(bg_r, bg_g, bg_b, 230);
            dyn_border = make_obs_color(acc_r, acc_g, acc_b, 160);
            dyn_prog = make_obs_color(acc_r, acc_g, acc_b, 255);
            dyn_prog_bg = make_obs_color(bg_r - 35, bg_g - 35, bg_b - 35, 200);
        }

        float inf = std::clamp(config_.dynamic_influence, 0.0f, 1.0f);
        active_bg = lerp_color(config_.bg_color, dyn_bg, inf);
        active_border = lerp_color(config_.border_color, dyn_border, inf);
        active_prog = lerp_color(config_.progress_color, dyn_prog, inf);
        active_prog_bg = lerp_color(config_.progress_bg, dyn_prog_bg, inf);
    }

    if (config_.card_opacity < 1.0f) {
        active_bg = multiply_alpha(active_bg, config_.card_opacity);
    }

    // 1. Background Card
    if (config_.show_background) {
        draw_solid_rect(0, 0, w, h, active_bg);
        if (config_.border_width > 0) {
            float bw = static_cast<float>(config_.border_width);
            // Draw 4 border segments without any corner overlap to prevent darker square blending
            draw_solid_rect(0, 0, w, bw, active_border);                      // Top full width
            draw_solid_rect(0, h - bw, w, bw, active_border);                  // Bottom full width
            draw_solid_rect(0, bw, bw, h - 2.0f * bw, active_border);          // Left between corners
            draw_solid_rect(w - bw, bw, bw, h - 2.0f * bw, active_border);      // Right between corners
        }
    }

    // 2. Cover Artwork
    float art_s = static_cast<float>(config_.art_size);
    float art_x = static_cast<float>(config_.padding);
    float art_y = (h - art_s) * 0.5f;

    if (image_renderer_.has_valid_image()) {
        image_renderer_.render(art_x, art_y, art_s, static_cast<float>(config_.art_radius),
                               config_.spin_art ? spin_angle_ : 0.0f);
    } else {
        // Fallback placeholder
        uint32_t ph_color = is_dark ? make_obs_color(30, 30, 36, 255) : make_obs_color(220, 220, 228, 255);
        draw_solid_rect(art_x, art_y, art_s, art_s, ph_color);
    }

    // 3. Text block
    float text_x = art_x + art_s + config_.padding;
    float text_y = art_y + 2.0f;

    text_renderer_.draw(title_tex_, text_x, text_y);
    text_y += (title_tex_.height > 0 ? title_tex_.height + 4.0f : config_.title_size + 4.0f);

    text_renderer_.draw(artist_tex_, text_x, text_y);
    text_y += (artist_tex_.height > 0 ? artist_tex_.height + 4.0f : config_.artist_size + 4.0f);

    if (album_tex_.texture) {
        text_renderer_.draw(album_tex_, text_x, text_y);
    }

    // 4. Progress bar
    if (config_.show_progress) {
        float bar_x = text_x;
        float bar_w = w - bar_x - config_.padding;
        float bar_h = static_cast<float>(config_.progress_height);
        float bar_y = art_y + art_s - bar_h - 2.0f;

        if (bar_w > 0.0f && bar_h > 0.0f) {
            // Background rail
            draw_solid_rect(bar_x, bar_y, bar_w, bar_h, active_prog_bg);

            // Filled progress
            float p = std::clamp(current_song_.timeline.progress, 0.0f, 1.0f);
            if (p > 0.0f) {
                draw_solid_rect(bar_x, bar_y, bar_w * p, bar_h, active_prog);
            }

            // Time text next to or above progress bar
            if (time_tex_.texture) {
                text_renderer_.draw(time_tex_, bar_x + bar_w - time_tex_.width, bar_y - time_tex_.height - 2.0f);
            }
        }
    }

    gs_matrix_pop();
}

} // namespace obsmv
