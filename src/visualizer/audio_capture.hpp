#pragma once

#include <obs-module.h>
#include <util/deque.h>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace obsmv {

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    void set_source(const std::string& source_name);
    std::string get_source() const;

    void tick(float seconds);

    // Call in tick to fetch latest PCM samples into output buffer
    bool fetch_samples(std::vector<float>& left_channel, std::vector<float>& right_channel, size_t sample_count);

    uint32_t get_sample_rate() const { return sample_rate_; }

    // Internal callback
    void on_audio_data(obs_source_t* src, const struct audio_data* data, bool muted);

private:
    void release_source();
    void try_connect_source();

    std::string source_name_;
    obs_weak_source_t* weak_source_{nullptr};

    std::mutex audio_mutex_;
    struct deque audio_circle_[2]{};
    uint32_t sample_rate_{48000};
    uint64_t last_check_time_{0};
};

} // namespace obsmv
