#include "media_detector.hpp"
#include <chrono>

namespace obsmv {

#if !defined(_WIN32) && !defined(__linux__)
// Fallback provider for macOS and other platforms:
// Native provider returns nullptr and MediaManager seamlessly synchronizes via OSMV Sync
std::unique_ptr<IMediaProvider> create_native_media_provider() {
    return nullptr;
}
#endif

MediaManager& MediaManager::instance() {
    static MediaManager s_mgr;
    return s_mgr;
}

MediaManager::MediaManager() {
    native_provider_ = create_native_media_provider();
    osmv_provider_ = create_osmv_sync_provider(osmv_json_path_);
}

MediaManager::~MediaManager() {
    stop();
}

void MediaManager::start() {
    if (running_.load()) return;
    running_.store(true);
    worker_thread_ = std::thread(&MediaManager::worker_loop, this);
}

void MediaManager::stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void MediaManager::set_mode(MediaSourceMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    mode_ = mode;
}

MediaSourceMode MediaManager::get_mode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mode_;
}

void MediaManager::set_osmv_json_path(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (osmv_json_path_ != path) {
        osmv_json_path_ = path;
        osmv_provider_ = create_osmv_sync_provider(osmv_json_path_);
    }
}

std::string MediaManager::get_osmv_json_path() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return osmv_json_path_;
}

SongInfo MediaManager::get_current_song() {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_song_;
}

void MediaManager::worker_loop() {
    while (running_.load()) {
        MediaSourceMode current_mode;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_mode = mode_;
        }

        SongInfo polled_song;
        bool found = false;

        if (current_mode == MediaSourceMode::NativeOnly) {
            if (native_provider_) {
                found = native_provider_->poll(polled_song);
            }
        } else if (current_mode == MediaSourceMode::OsmvSync) {
            if (osmv_provider_) {
                found = osmv_provider_->poll(polled_song);
            }
        } else {
            // Auto / Hybrid mode:
            // 1. Try Native first
            if (native_provider_ && native_provider_->poll(polled_song) && !polled_song.empty()) {
                found = true;
            }
            // 2. Fall back to OSMV Sync if native has no active song
            if (!found && osmv_provider_ && osmv_provider_->poll(polled_song) && !polled_song.empty()) {
                found = true;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (found) {
                current_song_ = std::move(polled_song);
            } else {
                current_song_.is_playing = false;
                current_song_.status = "closed";
            }
        }

        // Sleep ~250ms between polls for responsiveness without CPU load
        for (int i = 0; i < 5 && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

} // namespace obsmv

