#include "media_detector.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <vector>

namespace obsmv {

namespace {
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

std::string wide_to_utf8(std::wstring_view wstr) {
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), str.data(), size, nullptr, nullptr);
    return str;
}

std::vector<uint8_t> read_stream_bytes(const IRandomAccessStreamReference& stream_ref) {
    if (!stream_ref) return {};
    try {
        auto stream = stream_ref.OpenReadAsync().get();
        if (!stream) return {};
        uint64_t size = stream.Size();
        if (size == 0 || size > 10 * 1024 * 1024) return {};

        DataReader reader(stream.GetInputStreamAt(0));
        uint32_t loaded = reader.LoadAsync(static_cast<uint32_t>(size)).get();
        if (loaded == 0) return {};

        std::vector<uint8_t> buf(loaded);
        reader.ReadBytes(buf);
        reader.Close();
        stream.Close();
        return buf;
    } catch (...) {
        return {};
    }
}

uint64_t hash_bytes(const uint8_t* data, size_t size) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

std::string clean_app_name(const std::string& raw_aumid) {
    if (raw_aumid.empty()) return "Windows Media";
    std::string s = raw_aumid;
    auto bang = s.find('!');
    if (bang != std::string::npos) s = s.substr(0, bang);
    auto dot = s.rfind('.');
    if (dot != std::string::npos && dot + 1 < s.size()) {
        std::string tail = s.substr(dot + 1);
        if (tail != "exe" && tail != "EXE") return tail;
    }
    return s;
}

class WindowsMediaProvider final : public IMediaProvider {
public:
    WindowsMediaProvider() {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (...) {
            // Already initialized
        }
        ensure_manager();
    }

    bool poll(SongInfo& out_info) override {
        if (!ensure_manager()) return false;

        try {
            auto session = manager_.GetCurrentSession();
            if (!session) {
                last_sig_.clear();
                last_cover_data_.clear();
                return false;
            }

            try {
                out_info.player = clean_app_name(wide_to_utf8(std::wstring(session.SourceAppUserModelId())));
            } catch (...) {
                out_info.player = "Windows Media";
            }

            auto playback = session.GetPlaybackInfo();
            if (playback) {
                switch (playback.PlaybackStatus()) {
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                        out_info.status = "playing";
                        out_info.is_playing = true;
                        break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                        out_info.status = "paused";
                        out_info.is_playing = false;
                        break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
                        out_info.status = "stopped";
                        out_info.is_playing = false;
                        break;
                    default:
                        out_info.status = "closed";
                        out_info.is_playing = false;
                        break;
                }
            }

            // Timeline
            auto timeline = session.GetTimelineProperties();
            if (timeline) {
                out_info.timeline.position_ms = timeline.Position().count() / 10000;
                out_info.timeline.duration_ms = timeline.EndTime().count() / 10000;
                if (out_info.timeline.duration_ms > 0) {
                    out_info.timeline.progress = static_cast<float>(out_info.timeline.position_ms) /
                                                 static_cast<float>(out_info.timeline.duration_ms);
                }
            }

            auto media = session.TryGetMediaPropertiesAsync().get();
            if (!media) return !out_info.empty();

            out_info.title = wide_to_utf8(std::wstring(media.Title()));
            out_info.artist = wide_to_utf8(std::wstring(media.Artist()));
            out_info.album = wide_to_utf8(std::wstring(media.AlbumTitle()));

            std::string sig = out_info.title + "||" + out_info.artist + "||" + out_info.album;
            if (sig != last_sig_) {
                last_sig_ = sig;
                last_cover_data_ = read_stream_bytes(media.Thumbnail());
                last_cover_hash_ = hash_bytes(last_cover_data_.data(), last_cover_data_.size());
            }

            out_info.cover_data = last_cover_data_;
            out_info.has_cover = !last_cover_data_.empty();
            out_info.cover_hash = last_cover_hash_;

            return true;
        } catch (...) {
            return false;
        }
    }

private:
    bool ensure_manager() {
        if (manager_) return true;
        try {
            manager_ = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
            return manager_ != nullptr;
        } catch (...) {
            return false;
        }
    }

    GlobalSystemMediaTransportControlsSessionManager manager_{nullptr};
    std::string last_sig_;
    std::vector<uint8_t> last_cover_data_;
    uint64_t last_cover_hash_ = 0;
};

} // namespace

std::unique_ptr<IMediaProvider> create_native_media_provider() {
    return std::make_unique<WindowsMediaProvider>();
}

} // namespace obsmv
#else
namespace obsmv {
std::unique_ptr<IMediaProvider> create_native_media_provider() {
    return nullptr;
}
}
#endif
