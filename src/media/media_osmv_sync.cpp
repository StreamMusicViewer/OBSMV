#include "media_detector.hpp"
#include "../third_party/json/json.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace obsmv {

using json = nlohmann::json;

namespace {

uint64_t hash_bytes(const uint8_t* data, size_t size) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

std::vector<uint8_t> base64_decode(const std::string& in) {
    std::vector<uint8_t> out;
    static const int T[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (c == '=') break;
        if (T[c] == -1) continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::vector<uint8_t> read_file_bytes(const std::filesystem::path& p) {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) return {};
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

class OsmvSyncProvider final : public IMediaProvider {
public:
    explicit OsmvSyncProvider(std::string json_path) : custom_path_(std::move(json_path)) {}

    bool poll(SongInfo& out_info) override {
        // 1. Try reading the JSON file
        std::filesystem::path p = get_effective_json_path();
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) {
            auto mtime = std::filesystem::last_write_time(p, ec);
            if (!ec && mtime != last_mtime_) {
                last_mtime_ = mtime;
                if (parse_json_file(p, out_info)) {
                    cached_info_ = out_info;
                    return true;
                }
            } else if (!ec && !cached_info_.empty()) {
                out_info = cached_info_;
                return true;
            }
        }

        // 2. Try IPC if file wasn't available
        return poll_ipc(out_info);
    }

private:
    std::filesystem::path get_effective_json_path() const {
        if (!custom_path_.empty()) return custom_path_;
        // Look in current working directory / OSMV / shared / current_song.json
        std::filesystem::path candidates[] = {
            "current_song.json",
            "shared/current_song.json",
            "../OSMV/shared/current_song.json",
            "OSMV/shared/current_song.json"
        };
        for (const auto& c : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(c, ec)) return c;
        }
        return candidates[0];
    }

    bool parse_json_file(const std::filesystem::path& p, SongInfo& out) {
        try {
            std::ifstream f(p);
            if (!f) return false;
            json j = json::parse(f, nullptr, false);
            if (j.is_discarded() || j.is_null()) {
                out = SongInfo{};
                out.status = "closed";
                return false;
            }

            out.title = j.value("title", "");
            out.artist = j.value("artist", "");
            out.album = j.value("album", "");
            out.player = j.value("player", "");
            out.status = j.value("status", "playing");
            out.is_playing = (out.status == "playing");

            if (j.contains("timeline") && j["timeline"].is_object()) {
                out.timeline.position_ms = j["timeline"].value("position_ms", 0LL);
                out.timeline.duration_ms = j["timeline"].value("duration_ms", 0LL);
                if (out.timeline.duration_ms > 0) {
                    out.timeline.progress = static_cast<float>(out.timeline.position_ms) /
                                           static_cast<float>(out.timeline.duration_ms);
                }
            }

            // Cover artwork
            std::string b64;
            if (j.contains("thumbnail") && j["thumbnail"].is_string()) {
                b64 = j["thumbnail"];
            } else if (j.contains("art_base64") && j["art_base64"].is_string()) {
                b64 = j["art_base64"];
            }

            if (!b64.empty()) {
                out.cover_data = base64_decode(b64);
                out.has_cover = !out.cover_data.empty();
                out.cover_hash = hash_bytes(out.cover_data.data(), out.cover_data.size());
            } else if (j.contains("art_url") && j["art_url"].is_string()) {
                std::string url = j["art_url"];
                if (url.rfind("file://", 0) == 0) {
                    std::string local_path = url.substr(7);
#ifdef _WIN32
                    if (local_path.size() >= 3 && local_path[0] == '/' && local_path[2] == ':') {
                        local_path = local_path.substr(1);
                    }
#endif
                    out.cover_data = read_file_bytes(local_path);
                    out.has_cover = !out.cover_data.empty();
                    out.cover_hash = hash_bytes(out.cover_data.data(), out.cover_data.size());
                }
            }

            return true;
        } catch (...) {
            return false;
        }
    }

    bool poll_ipc(SongInfo& out) {
#ifdef _WIN32
        const char* pipe_name = "\\\\.\\pipe\\osmv-ipc-v3";
        HANDLE pipe = CreateFileA(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                  OPEN_EXISTING, 0, nullptr);
        if (pipe == INVALID_HANDLE_VALUE) return false;

        std::string req = "{\"cmd\":\"get_state\"}\n";
        DWORD written = 0;
        if (!WriteFile(pipe, req.data(), static_cast<DWORD>(req.size()), &written, nullptr)) {
            CloseHandle(pipe);
            return false;
        }

        std::string resp;
        char buf[4096];
        DWORD read_bytes = 0;
        while (ReadFile(pipe, buf, sizeof(buf) - 1, &read_bytes, nullptr) && read_bytes > 0) {
            buf[read_bytes] = '\0';
            resp.append(buf, read_bytes);
            if (resp.find('\n') != std::string::npos) break;
        }
        CloseHandle(pipe);

        try {
            json j = json::parse(resp, nullptr, false);
            if (j.is_discarded() || !j.contains("now_playing")) return false;
            json np = j["now_playing"];
            out.title = np.value("title", "");
            out.artist = np.value("artist", "");
            out.album = np.value("album", "");
            out.player = np.value("player", "");
            out.status = np.value("status", "playing");
            out.is_playing = (out.status == "playing");
            return true;
        } catch (...) {
            return false;
        }
#else
        return false;
#endif
    }

    std::string custom_path_;
    std::filesystem::file_time_type last_mtime_{};
    SongInfo cached_info_{};
};

} // namespace

std::unique_ptr<IMediaProvider> create_osmv_sync_provider(const std::string& json_path) {
    return std::make_unique<OsmvSyncProvider>(json_path);
}

} // namespace obsmv

