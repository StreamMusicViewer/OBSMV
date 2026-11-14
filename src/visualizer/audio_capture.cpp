#include "audio_capture.hpp"
#include <media-io/audio-io.h>
#include <util/platform.h>
#include <cstring>
#include <algorithm>

namespace obsmv {

static void audio_capture_cb(void* param, obs_source_t* src, const struct audio_data* data, bool muted) {
    auto* cap = static_cast<AudioCapture*>(param);
    if (cap) {
        cap->on_audio_data(src, data, muted);
    }
}

AudioCapture::AudioCapture() {
    circlebuf_init(&audio_circle_[0]);
    circlebuf_init(&audio_circle_[1]);

    obs_audio_info ai{};
    if (obs_get_audio_info(&ai)) {
        sample_rate_ = ai.samples_per_sec;
    }
}

AudioCapture::~AudioCapture() {
    release_source();
    circlebuf_free(&audio_circle_[0]);
    circlebuf_free(&audio_circle_[1]);
}

void AudioCapture::release_source() {
    if (weak_source_) {
        obs_source_t* src = obs_weak_source_get_source(weak_source_);
        if (src) {
            obs_source_remove_audio_capture_callback(src, audio_capture_cb, this);
            obs_source_release(src);
        }
        obs_weak_source_release(weak_source_);
        weak_source_ = nullptr;
    }
}

void AudioCapture::try_connect_source() {
    if (source_name_.empty() || source_name_ == "none" || weak_source_) return;

    obs_source_t* src = obs_get_source_by_name(source_name_.c_str());
    if (src) {
        weak_source_ = obs_source_get_weak_source(src);
        obs_source_add_audio_capture_callback(src, audio_capture_cb, this);
        blog(LOG_INFO, "[OBSMV] Successfully hooked audio capture to source '%s'", source_name_.c_str());
        obs_source_release(src);
    }
}

void AudioCapture::set_source(const std::string& name) {
    if (source_name_ == name && weak_source_ != nullptr) return;

    std::lock_guard<std::mutex> lock(audio_mutex_);
    release_source();
    source_name_ = name;

    try_connect_source();

    circlebuf_free(&audio_circle_[0]);
    circlebuf_free(&audio_circle_[1]);
    circlebuf_init(&audio_circle_[0]);
    circlebuf_init(&audio_circle_[1]);
}

std::string AudioCapture::get_source() const {
    return source_name_;
}

void AudioCapture::tick(float) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    // Auto-reconnect if source wasn't ready during initialization
    if (!source_name_.empty() && source_name_ != "none" && !weak_source_) {
        uint64_t now_ns = os_gettime_ns();
        if (now_ns - last_check_time_ > 1000000000ULL) { // Check every 1s
            last_check_time_ = now_ns;
            try_connect_source();
        }
    }
}

void AudioCapture::on_audio_data(obs_source_t*, const struct audio_data* data, bool muted) {
    if (!data || data->frames == 0) return;

    std::lock_guard<std::mutex> lock(audio_mutex_);

    size_t write_bytes = data->frames * sizeof(float);
    if (muted) {
        circlebuf_push_back_zero(&audio_circle_[0], write_bytes);
        circlebuf_push_back_zero(&audio_circle_[1], write_bytes);
    } else {
        circlebuf_push_back(&audio_circle_[0], data->data[0], write_bytes);
        if (data->data[1]) {
            circlebuf_push_back(&audio_circle_[1], data->data[1], write_bytes);
        } else {
            // Mono duplicate
            circlebuf_push_back(&audio_circle_[1], data->data[0], write_bytes);
        }
    }

    // Keep max ~8192 frames (approx 170ms) to ensure low latency
    const size_t max_bytes = 8192 * sizeof(float);
    if (audio_circle_[0].size > max_bytes) {
        size_t excess = audio_circle_[0].size - max_bytes;
        circlebuf_pop_front(&audio_circle_[0], nullptr, excess);
        circlebuf_pop_front(&audio_circle_[1], nullptr, excess);
    }
}

bool AudioCapture::fetch_samples(std::vector<float>& left_channel,
                                std::vector<float>& right_channel,
                                size_t sample_count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    size_t bytes_needed = sample_count * sizeof(float);
    left_channel.assign(sample_count, 0.0f);
    right_channel.assign(sample_count, 0.0f);

    if (audio_circle_[0].size == 0) {
        return false;
    }

    if (audio_circle_[0].size < bytes_needed) {
        size_t available_bytes = audio_circle_[0].size;
        size_t available_samples = available_bytes / sizeof(float);
        size_t offset = sample_count - available_samples;

        circlebuf_peek_front(&audio_circle_[0], left_channel.data() + offset, available_bytes);
        circlebuf_peek_front(&audio_circle_[1], right_channel.data() + offset, available_bytes);

        circlebuf_pop_front(&audio_circle_[0], nullptr, available_bytes);
        circlebuf_pop_front(&audio_circle_[1], nullptr, available_bytes);
    } else {
        // Discard ancient samples beyond the sample window so we always read real-time audio
        if (audio_circle_[0].size > bytes_needed * 2) {
            size_t excess = audio_circle_[0].size - bytes_needed * 2;
            circlebuf_pop_front(&audio_circle_[0], nullptr, excess);
            circlebuf_pop_front(&audio_circle_[1], nullptr, excess);
        }

        circlebuf_peek_front(&audio_circle_[0], left_channel.data(), bytes_needed);
        circlebuf_peek_front(&audio_circle_[1], right_channel.data(), bytes_needed);

        // Advance by ~800 samples (1 video frame at 48kHz / 60 FPS) for smooth streaming
        size_t advance = std::min(bytes_needed / 2, audio_circle_[0].size);
        circlebuf_pop_front(&audio_circle_[0], nullptr, advance);
        circlebuf_pop_front(&audio_circle_[1], nullptr, advance);
    }

    return true;
}

} // namespace obsmv
