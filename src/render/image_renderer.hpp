#pragma once

#include <obs-module.h>
#include <graphics/graphics.h>
#include <vector>
#include <cstdint>

namespace obsmv {

class ImageRenderer {
public:
    ImageRenderer();
    ~ImageRenderer();

    // Updates artwork texture from image file bytes (PNG, JPEG, etc.)
    bool update_image(const std::vector<uint8_t>& image_bytes, uint64_t hash);

    void free_texture();

    // Renders the cover artwork
    void render(float x, float y, float size, float corner_radius = 8.0f, float rotation_degrees = 0.0f);

    uint32_t get_accent_color() const { return accent_color_; }
    uint32_t get_primary_color() const { return primary_color_; }
    uint32_t get_dominant_color() const { return accent_color_; }
    bool is_dark_artwork() const;
    bool has_valid_image() const { return texture_ != nullptr; }

private:
    void compute_palette(const uint8_t* rgba, int w, int h);

    gs_texture_t* texture_{nullptr};
    uint32_t width_{0};
    uint32_t height_{0};
    uint64_t current_hash_{0};
    uint32_t accent_color_{0xFFFF00AA};
    uint32_t primary_color_{0xFF141212};
};

} // namespace obsmv
