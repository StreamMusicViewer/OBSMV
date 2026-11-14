#include "text_renderer.hpp"
#include "color_utils.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb/stb_truetype.h"

#include <graphics/matrix4.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wingdi.h>
#define strcasecmp _stricmp
#endif

namespace obsmv {

static uint32_t utf8_next_codepoint(const char*& p) {
    unsigned char c = static_cast<unsigned char>(*p);
    if (c == 0) return 0;
    p++;
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0) {
        uint32_t cp = (c & 0x1F) << 6;
        if (*p) cp |= (*p++ & 0x3F);
        return cp;
    }
    if ((c & 0xF0) == 0xE0) {
        uint32_t cp = (c & 0x0F) << 12;
        if (*p) cp |= (*p++ & 0x3F) << 6;
        if (*p) cp |= (*p++ & 0x3F);
        return cp;
    }
    if ((c & 0xF8) == 0xF0) {
        uint32_t cp = (c & 0x07) << 18;
        if (*p) cp |= (*p++ & 0x3F) << 12;
        if (*p) cp |= (*p++ & 0x3F) << 6;
        if (*p) cp |= (*p++ & 0x3F);
        return cp;
    }
    return '?';
}

TextRenderer::TextRenderer() {}

TextRenderer::~TextRenderer() {}

bool TextRenderer::load_font_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    font_buffer_ = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
    font_loaded_ = !font_buffer_.empty();
    if (font_loaded_) {
        blog(LOG_INFO, "[OBSMV] Loaded font: %s (%zu bytes)", path.c_str(), font_buffer_.size());
    }
    return font_loaded_;
}

bool TextRenderer::load_font_memory(const uint8_t* data, size_t size) {
    if (!data || size == 0) return false;
    font_buffer_.assign(data, data + size);
    font_loaded_ = true;
    return true;
}

bool TextRenderer::load_system_font(const std::string& font_face) {
    if (font_face.empty()) return false;

#ifdef _WIN32
    HDC hdc = CreateCompatibleDC(nullptr);
    if (!hdc) return false;

    LOGFONTW lf{};
    lf.lfCharSet = DEFAULT_CHARSET;
    MultiByteToWideChar(CP_UTF8, 0, font_face.c_str(), -1, lf.lfFaceName, LF_FACESIZE - 1);

    HFONT hFont = CreateFontIndirectW(&lf);
    if (!hFont) {
        DeleteDC(hdc);
        return false;
    }

    HGDIOBJ oldFont = SelectObject(hdc, hFont);
    DWORD size = GetFontData(hdc, 0, 0, nullptr, 0);

    bool loaded = false;
    if (size != GDI_ERROR && size > 0) {
        std::vector<uint8_t> buffer(size);
        DWORD read_bytes = GetFontData(hdc, 0, 0, buffer.data(), size);
        if (read_bytes == size) {
            loaded = load_font_memory(buffer.data(), buffer.size());
        }
    }

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
    DeleteDC(hdc);

    if (loaded) {
        blog(LOG_INFO, "[OBSMV] Successfully loaded system font '%s' (%zu bytes)",
             font_face.c_str(), font_buffer_.size());
        return true;
    }
#endif

    // Fallback: search Windows Fonts directory
    std::filesystem::path win_fonts = "C:/Windows/Fonts";
    std::error_code ec;
    if (std::filesystem::exists(win_fonts, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(win_fonts, ec)) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().stem().string();
                std::string ext = entry.path().extension().string();
                if (ext == ".ttf" || ext == ".otf") {
                    if (strcasecmp(fname.c_str(), font_face.c_str()) == 0) {
                        return load_font_file(entry.path().string());
                    }
                }
            }
        }
    }

    return false;
}

void TextRenderer::free_text_texture(TextTexture& tt) {
    if (tt.texture) {
        obs_enter_graphics();
        gs_texture_destroy(tt.texture);
        obs_leave_graphics();
        tt.texture = nullptr;
    }
    tt.tex_width = 0;
    tt.tex_height = 0;
    tt.width = 0;
    tt.height = 0;
    tt.text.clear();
}

void TextRenderer::update_text_texture(TextTexture& tt,
                                      const std::string& text,
                                      float font_size,
                                      uint32_t rgba_color,
                                      uint32_t max_width) {
    if (text.empty() || !font_loaded_ || font_size <= 0.0f) {
        free_text_texture(tt);
        return;
    }

    // Skip re-rasterization if identical
    if (tt.texture && tt.text == text && tt.font_size == font_size && tt.color == rgba_color) {
        return;
    }

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, font_buffer_.data(), 0)) {
        return;
    }

    // 2x Retina supersampling for ultra-crisp typography
    constexpr float supersample = 2.0f;
    float scaled_font_size = font_size * supersample;
    float scale = stbtt_ScaleForPixelHeight(&font, scaled_font_size);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
    int baseline = static_cast<int>(ascent * scale);
    int line_height = static_cast<int>((ascent - descent + line_gap) * scale);
    if (line_height <= 0) line_height = static_cast<int>(scaled_font_size);

    // Measure total width
    int total_width = 0;
    int scaled_max_w = (max_width > 0) ? static_cast<int>(max_width * supersample) : 0;

    const char* p_measure = text.c_str();
    while (*p_measure) {
        uint32_t cp = utf8_next_codepoint(p_measure);
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);
        total_width += static_cast<int>(advance * scale);
        if (scaled_max_w > 0 && total_width > scaled_max_w) {
            total_width = scaled_max_w;
            break;
        }
    }

    if (total_width <= 0) total_width = 1;

    // Add extra 4px safety margins to avoid glyph edge clipping
    int pad_x = 2;
    int pad_y = 2;
    int tex_w = total_width + (pad_x * 2);
    int tex_h = line_height + (pad_y * 2);

    // Explicit 4-byte RGBA buffer: byte 0 = R, byte 1 = G, byte 2 = B, byte 3 = A
    std::vector<uint8_t> rgba_bytes(tex_w * tex_h * 4, 0);

    uint8_t cr, cg, cb, ca;
    unpack_obs_color(rgba_color, cr, cg, cb, ca);

    int cur_x = pad_x;
    int base_y = baseline + pad_y;

    const char* p_draw = text.c_str();
    while (*p_draw) {
        uint32_t cp = utf8_next_codepoint(p_draw);
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);

        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&font, cp, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

        int glyph_w = c_x2 - c_x1;
        int glyph_h = c_y2 - c_y1;
        if (glyph_w > 0 && glyph_h > 0) {
            std::vector<uint8_t> mono(glyph_w * glyph_h);
            stbtt_MakeCodepointBitmap(&font, mono.data(), glyph_w, glyph_h, glyph_w, scale, scale, cp);

            int dst_x_start = cur_x + c_x1;
            int dst_y_start = base_y + c_y1;

            for (int gy = 0; gy < glyph_h; ++gy) {
                int dy = dst_y_start + gy;
                if (dy < 0 || dy >= tex_h) continue;

                for (int gx = 0; gx < glyph_w; ++gx) {
                    int dx = dst_x_start + gx;
                    if (dx < 0 || dx >= tex_w) continue;

                    uint8_t alpha = mono[gy * glyph_w + gx];
                    if (alpha == 0) continue;

                    uint8_t final_a = static_cast<uint8_t>((alpha * ca) / 255);
                    int p_idx = (dy * tex_w + dx) * 4;

                    if (final_a > rgba_bytes[p_idx + 3]) {
                        rgba_bytes[p_idx + 0] = cr;
                        rgba_bytes[p_idx + 1] = cg;
                        rgba_bytes[p_idx + 2] = cb;
                        rgba_bytes[p_idx + 3] = final_a;
                    }
                }
            }
        }

        cur_x += static_cast<int>(advance * scale);
        if (scaled_max_w > 0 && cur_x >= (scaled_max_w + pad_x)) break;
    }

    obs_enter_graphics();

    if (tt.texture && (tt.tex_width != static_cast<uint32_t>(tex_w) || tt.tex_height != static_cast<uint32_t>(tex_h))) {
        gs_texture_destroy(tt.texture);
        tt.texture = nullptr;
    }

    const uint8_t* raw_ptr = rgba_bytes.data();
    if (!tt.texture) {
        tt.texture = gs_texture_create(tex_w, tex_h, GS_RGBA, 1, &raw_ptr, GS_DYNAMIC);
    } else {
        gs_texture_set_image(tt.texture, raw_ptr, tex_w * 4, false);
    }

    obs_leave_graphics();

    tt.tex_width = static_cast<uint32_t>(tex_w);
    tt.tex_height = static_cast<uint32_t>(tex_h);
    tt.width = static_cast<uint32_t>(std::ceil(tex_w / supersample));
    tt.height = static_cast<uint32_t>(std::ceil(tex_h / supersample));
    tt.text = text;
    tt.font_size = font_size;
    tt.color = rgba_color;
}

void TextRenderer::draw(const TextTexture& tt, float x, float y) {
    if (!tt.texture || tt.width == 0 || tt.height == 0) return;

    gs_effect_t* effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    if (!effect) return;

    gs_eparam_t* image_param = gs_effect_get_param_by_name(effect, "image");
    gs_technique_t* tech = gs_effect_get_technique(effect, "Draw");
    if (!image_param || !tech) return;

    gs_blend_state_push();
    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    gs_effect_set_texture(image_param, tt.texture);

    gs_matrix_push();
    gs_matrix_translate3f(x, y, 0.0f);

    size_t passes = gs_technique_begin(tech);
    for (size_t i = 0; i < passes; ++i) {
        gs_technique_begin_pass(tech, i);
        // Draw at 1x display width and height; GPU bilinear downsamples from 2x texture
        gs_draw_sprite(tt.texture, 0, tt.width, tt.height);
        gs_technique_end_pass(tech);
    }
    gs_technique_end(tech);

    gs_matrix_pop();
    gs_blend_state_pop();
}

} // namespace obsmv
