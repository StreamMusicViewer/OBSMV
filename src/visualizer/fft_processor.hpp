#pragma once

#include "../third_party/kissfft/kiss_fftr.h"
#include <vector>
#include <cstdint>

namespace obsmv {

enum class VisualizerSmoothing {
    None = 0,
    Monstercat,
    Gravity
};

struct VisualizerConfig {
    int bar_count = 32;
    float min_freq = 20.0f;
    float max_freq = 20000.0f;
    float scale_boost = 1.0f;
    float gravity = 0.85f;
    VisualizerSmoothing smoothing = VisualizerSmoothing::Monstercat;
    bool stereo = false;
};

class FFTProcessor {
public:
    explicit FFTProcessor(size_t window_size = 1024);
    ~FFTProcessor();

    void configure(const VisualizerConfig& cfg);

    void process(const std::vector<float>& left_samples,
                 const std::vector<float>& right_samples,
                 uint32_t sample_rate,
                 float dt_seconds,
                 bool has_audio);

    const std::vector<float>& get_left_bins() const { return current_left_bins_; }
    const std::vector<float>& get_right_bins() const { return current_right_bins_; }

private:
    void init_kiss_fft(size_t window_size);
    void compute_channel(const std::vector<float>& in_samples,
                         std::vector<float>& out_bins,
                         std::vector<float>& prev_bins,
                         uint32_t sample_rate,
                         float dt,
                         bool has_audio);

    size_t window_size_;
    kiss_fftr_cfg kiss_cfg_{nullptr};
    std::vector<float> window_weights_;
    std::vector<float> time_buffer_;
    std::vector<kiss_fft_cpx> freq_buffer_;

    VisualizerConfig config_;
    std::vector<float> current_left_bins_;
    std::vector<float> current_right_bins_;
    std::vector<float> prev_left_bins_;
    std::vector<float> prev_right_bins_;
};

} // namespace obsmv
