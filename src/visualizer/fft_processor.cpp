#include "fft_processor.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace obsmv {

FFTProcessor::FFTProcessor(size_t window_size) : window_size_(window_size) {
    init_kiss_fft(window_size_);
}

FFTProcessor::~FFTProcessor() {
    if (kiss_cfg_) {
        kiss_fftr_free(kiss_cfg_);
        kiss_cfg_ = nullptr;
    }
}

void FFTProcessor::init_kiss_fft(size_t window_size) {
    if (kiss_cfg_) {
        kiss_fftr_free(kiss_cfg_);
    }
    window_size_ = window_size;
    kiss_cfg_ = kiss_fftr_alloc(static_cast<int>(window_size_), 0, nullptr, nullptr);

    window_weights_.resize(window_size_);
    for (size_t i = 0; i < window_size_; ++i) {
        window_weights_[i] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / (window_size_ - 1)));
    }

    time_buffer_.resize(window_size_, 0.0f);
    freq_buffer_.resize(window_size_ / 2 + 1);
}

void FFTProcessor::configure(const VisualizerConfig& cfg) {
    config_ = cfg;
    current_left_bins_.resize(config_.bar_count, 0.0f);
    current_right_bins_.resize(config_.bar_count, 0.0f);
    prev_left_bins_.resize(config_.bar_count, 0.0f);
    prev_right_bins_.resize(config_.bar_count, 0.0f);
}

void FFTProcessor::compute_channel(const std::vector<float>& in_samples,
                                  std::vector<float>& out_bins,
                                  std::vector<float>& prev_bins,
                                  uint32_t sample_rate,
                                  float dt,
                                  bool has_audio) {
    if (config_.bar_count <= 0) return;

    out_bins.resize(config_.bar_count, 0.0f);
    prev_bins.resize(config_.bar_count, 0.0f);

    float decay_rate = (1.0f - config_.gravity * 0.4f) * dt * 5.0f;
    if (decay_rate < 0.01f) decay_rate = 0.01f;

    if (!has_audio || in_samples.empty() || !kiss_cfg_) {
        // Smoothly decay existing bars to 0
        for (int b = 0; b < config_.bar_count; ++b) {
            prev_bins[b] = std::max(0.0f, prev_bins[b] - decay_rate);
            out_bins[b] = prev_bins[b];
        }
        return;
    }

    size_t copy_len = std::min(in_samples.size(), window_size_);
    for (size_t i = 0; i < copy_len; ++i) {
        time_buffer_[i] = in_samples[i] * window_weights_[i];
    }
    for (size_t i = copy_len; i < window_size_; ++i) {
        time_buffer_[i] = 0.0f;
    }

    // FFT
    kiss_fftr(kiss_cfg_, time_buffer_.data(), freq_buffer_.data());

    float freq_resolution = static_cast<float>(sample_rate > 0 ? sample_rate : 48000) / static_cast<float>(window_size_);
    size_t num_fft_bins = window_size_ / 2;

    float log_min = std::log10(std::max(10.0f, config_.min_freq));
    float log_max = std::log10(std::min(static_cast<float>(sample_rate) / 2.0f, config_.max_freq));

    for (int b = 0; b < config_.bar_count; ++b) {
        float f_low = std::pow(10.0f, log_min + (log_max - log_min) * (static_cast<float>(b) / config_.bar_count));
        float f_high = std::pow(10.0f, log_min + (log_max - log_min) * (static_cast<float>(b + 1) / config_.bar_count));

        size_t idx_start = std::max<size_t>(1, static_cast<size_t>(f_low / freq_resolution));
        size_t idx_end = std::min<size_t>(num_fft_bins, static_cast<size_t>(std::ceil(f_high / freq_resolution)));
        if (idx_end <= idx_start) idx_end = idx_start + 1;

        float magnitude_sum = 0.0f;
        for (size_t i = idx_start; i < idx_end && i < num_fft_bins; ++i) {
            float re = freq_buffer_[i].r;
            float im = freq_buffer_[i].i;
            magnitude_sum += std::sqrt(re * re + im * im);
        }

        float avg_mag = magnitude_sum / static_cast<float>(idx_end - idx_start);

        // Normalize with boost
        float freq_boost = 1.0f + 0.5f * std::log2(static_cast<float>(b + 1));
        float raw_val = (avg_mag / (window_size_ * 0.08f)) * config_.scale_boost * freq_boost;
        raw_val = std::clamp(raw_val, 0.0f, 1.0f);

        // Smoothing
        if (config_.smoothing == VisualizerSmoothing::Monstercat) {
            if (raw_val > prev_bins[b]) {
                out_bins[b] = raw_val;
            } else {
                out_bins[b] = std::max(0.0f, prev_bins[b] - decay_rate);
            }
        } else if (config_.smoothing == VisualizerSmoothing::Gravity) {
            float grav_decay = dt * config_.gravity * 4.0f;
            out_bins[b] = std::max(0.0f, std::max(raw_val, prev_bins[b] - grav_decay));
        } else {
            out_bins[b] = raw_val;
        }

        prev_bins[b] = out_bins[b];
    }

    if (config_.smoothing == VisualizerSmoothing::Monstercat && config_.bar_count > 3) {
        std::vector<float> smoothed = out_bins;
        for (int b = 1; b < config_.bar_count - 1; ++b) {
            smoothed[b] = out_bins[b] * 0.6f + (out_bins[b - 1] + out_bins[b + 1]) * 0.2f;
        }
        out_bins = std::move(smoothed);
    }
}

void FFTProcessor::process(const std::vector<float>& left_samples,
                          const std::vector<float>& right_samples,
                          uint32_t sample_rate,
                          float dt_seconds,
                          bool has_audio) {
    compute_channel(left_samples, current_left_bins_, prev_left_bins_, sample_rate, dt_seconds, has_audio);
    if (config_.stereo) {
        compute_channel(right_samples, current_right_bins_, prev_right_bins_, sample_rate, dt_seconds, has_audio);
    } else {
        current_right_bins_ = current_left_bins_;
    }
}

} // namespace obsmv
