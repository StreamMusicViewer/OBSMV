#pragma once

#include "text_renderer.hpp"
#include "image_renderer.hpp"
#include "color_utils.hpp"
#include "../media/song_info.hpp"

namespace obsmv {

enum class CardThemeMode {
    Auto = 0, // Adapts to album cover brightness
    Dark,     // Always dark glass
    Light     // Always light frosted glass
};

struct CardConfig {
    // Layout & Dimensions
    int width = 450;
    int height = 120;
    int padding = 16;
    int art_size = 80;
    int art_radius = 8;
    bool spin_art = false;

    // Theme Mode
    CardThemeMode theme_mode = CardThemeMode::Auto;

    // Background & Border
    bool show_background = true;
    uint32_t bg_color = make_obs_color(18, 18, 20, 230);     // Sleek dark glass
    uint32_t border_color = make_obs_color(255, 255, 255, 38); // Subtle outline
    int border_width = 1;
    int border_radius = 12;
    float card_opacity = 0.90f; // 0.0 (transparent) to 1.0 (opaque)

    // Typography
    std::string font_face = "Inter";
    std::string font_path;
    float title_size = 18.0f;
    float artist_size = 14.0f;
    float album_size = 12.0f;
    uint32_t title_color = make_obs_color(255, 255, 255, 255);
    uint32_t artist_color = make_obs_color(179, 179, 179, 255);
    uint32_t album_color = make_obs_color(138, 155, 191, 255);
    std::string title_format = "{title}";
    std::string artist_format = "{artist}";
    std::string album_format = "{album}";
    bool show_album = true;

    // Progress Bar
    bool show_progress = true;
    int progress_height = 4;
    uint32_t progress_color = make_obs_color(170, 0, 255, 255);
    uint32_t progress_bg = make_obs_color(255, 255, 255, 45);
    bool show_time = true;
    float time_size = 10.0f;
    uint32_t time_color = make_obs_color(160, 160, 160, 255);

    // Dynamic Color Mode & Photoshop-style Influence
    bool dynamic_color = true;
    float dynamic_influence = 1.0f; // 0.0 (custom color only) to 1.0 (full album dominant)
};

class CardRenderer {
public:
    CardRenderer();
    ~CardRenderer();

    void configure(const CardConfig& cfg);
    void update(const SongInfo& song, float dt_seconds);
    void render(float origin_x, float origin_y);

    uint32_t get_accent_color() const { return image_renderer_.get_accent_color(); }
    uint32_t get_primary_color() const { return image_renderer_.get_primary_color(); }
    uint32_t get_dominant_color() const { return image_renderer_.get_accent_color(); }

private:
    std::string format_string(const std::string& pattern, const SongInfo& song);
    std::string format_time(int64_t ms);

    void draw_solid_rect(float x, float y, float w, float h, uint32_t rgba);

    CardConfig config_;
    SongInfo current_song_;

    TextRenderer text_renderer_;
    ImageRenderer image_renderer_;

    TextTexture title_tex_;
    TextTexture artist_tex_;
    TextTexture album_tex_;
    TextTexture time_tex_;

    float spin_angle_{0.0f};
};

} // namespace obsmv
