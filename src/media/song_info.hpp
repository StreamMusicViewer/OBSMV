#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace obsmv {

struct SongTimeline {
    int64_t position_ms = 0;
    int64_t duration_ms = 0;
    float progress = 0.0f; // 0.0f - 1.0f
};

struct SongInfo {
    std::string title;
    std::string artist;
    std::string album;
    std::string player;
    std::string status = "closed"; // "playing", "paused", "stopped", "closed"
    bool is_playing = false;

    // Artwork
    bool has_cover = false;
    std::vector<uint8_t> cover_data; // Raw image file bytes (PNG/JPEG)
    uint64_t cover_hash = 0;
    uint32_t dominant_color = 0xFF7C5CFF; // Default accent (RGBA)

    // Timeline / Progress
    SongTimeline timeline;

    bool empty() const {
        return title.empty() && artist.empty();
    }

    bool equals_metadata(const SongInfo& other) const {
        return title == other.title &&
               artist == other.artist &&
               album == other.album &&
               status == other.status &&
               cover_hash == other.cover_hash;
    }
};

enum class MediaSourceMode {
    Auto = 0,    // Detect SMTC / MPRIS / OSMV automatically
    NativeOnly,  // Direct OS APIs only (Windows SMTC, Linux MPRIS, macOS)
    OsmvSync     // OSMV Daemon IPC / current_song.json file
};

} // namespace obsmv

