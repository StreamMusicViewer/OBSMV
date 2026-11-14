#pragma once

#include "song_info.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <string>

namespace obsmv {

class IMediaProvider {
public:
    virtual ~IMediaProvider() = default;
    virtual bool poll(SongInfo& out_info) = 0;
};

class MediaManager {
public:
    static MediaManager& instance();

    void start();
    void stop();

    void set_mode(MediaSourceMode mode);
    MediaSourceMode get_mode() const;

    void set_osmv_json_path(const std::string& path);
    std::string get_osmv_json_path() const;

    SongInfo get_current_song();

private:
    MediaManager();
    ~MediaManager();

    void worker_loop();

    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    mutable std::mutex mutex_;
    SongInfo current_song_;
    MediaSourceMode mode_{MediaSourceMode::Auto};
    std::string osmv_json_path_;

    std::unique_ptr<IMediaProvider> native_provider_;
    std::unique_ptr<IMediaProvider> osmv_provider_;
};

std::unique_ptr<IMediaProvider> create_native_media_provider();
std::unique_ptr<IMediaProvider> create_osmv_sync_provider(const std::string& json_path);

} // namespace obsmv

