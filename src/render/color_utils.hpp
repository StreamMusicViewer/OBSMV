#pragma once
#include <cstdint>
#include <algorithm>

namespace obsmv {

// Libobs color integers (when passed to vec4_from_rgba or returned by obs_data_get_int):
// In little-endian memory: Byte 0 = Red, Byte 1 = Green, Byte 2 = Blue, Byte 3 = Alpha.
inline uint32_t make_obs_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(g) << 8)  |
           (static_cast<uint32_t>(r));
}

inline void unpack_obs_color(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
    r = static_cast<uint8_t>(color & 0xFF);
    g = static_cast<uint8_t>((color >> 8) & 0xFF);
    b = static_cast<uint8_t>((color >> 16) & 0xFF);
    a = static_cast<uint8_t>((color >> 24) & 0xFF);
}

// Linearly interpolate between two colors (Photoshop-like influence / opacity blending)
inline uint32_t lerp_color(uint32_t c1, uint32_t c2, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    uint8_t r1, g1, b1, a1, r2, g2, b2, a2;
    unpack_obs_color(c1, r1, g1, b1, a1);
    unpack_obs_color(c2, r2, g2, b2, a2);
    uint8_t r = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(r1 + (r2 - r1) * t), 0, 255));
    uint8_t g = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(g1 + (g2 - g1) * t), 0, 255));
    uint8_t b = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(b1 + (b2 - b1) * t), 0, 255));
    uint8_t a = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(a1 + (a2 - a1) * t), 0, 255));
    return make_obs_color(r, g, b, a);
}

// Multiply the alpha channel by a factor (0.0f - 1.0f)
inline uint32_t multiply_alpha(uint32_t c, float factor) {
    uint8_t r, g, b, a;
    unpack_obs_color(c, r, g, b, a);
    uint8_t final_a = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(a * factor), 0, 255));
    return make_obs_color(r, g, b, final_a);
}

} // namespace obsmv
