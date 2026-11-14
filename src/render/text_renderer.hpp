#pragma once

#include <obs-module.h>
#include <graphics/graphics.h>
#include <string>
#include <vector>
#include <cstdint>

namespace obsmv {

struct TextTexture {
    gs_texture_t* texture = nullptr;
    uint32_t tex_width = 0;   // Texture pixel width (supersampled)
    uint32_t tex_height = 0;  // Texture pixel height (supersampled)
    uint32_t width = 0;       // Display layout width
    uint32_t height = 0;      // Display layout height
    std::string text;
    float font_size = 0.0f;
    uint32_t color = 0;
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Loads a TTF/OTF font file (from disk or memory)
    bool load_font_file(const std::string& path);
    bool load_font_memory(const uint8_t* data, size_t size);
    bool load_system_font(const std::string& font_face);

    // Updates or generates a texture for the given text string
    void update_text_texture(TextTexture& tt,
                             const std::string& text,
                             float font_size,
                             uint32_t rgba_color,
                             uint32_t max_width = 0);

    // Frees a text texture
    void free_text_texture(TextTexture& tt);

    // Renders the text texture at (x, y)
    void draw(const TextTexture& tt, float x, float y);

private:
    std::vector<uint8_t> font_buffer_;
    bool font_loaded_{false};
};

} // namespace obsmv
