#pragma once

#include <obs-module.h>
#include "../visualizer/audio_capture.hpp"
#include "../visualizer/fft_processor.hpp"
#include "../visualizer/visualizer_renderer.hpp"
#include "../render/image_renderer.hpp"
#include <mutex>

namespace obsmv {

class SourceVisualizer {
public:
    SourceVisualizer(obs_data_t* settings, obs_source_t* source);
    ~SourceVisualizer();

    void update(obs_data_t* settings);
    void tick(float seconds);
    void render(gs_effect_t* effect);

    uint32_t get_width() const { return width_; }
    uint32_t get_height() const { return height_; }

    static void register_source();

private:
    obs_source_t* source_{nullptr};
    std::mutex mutex_;

    AudioCapture audio_capture_;
    FFTProcessor fft_processor_;
    VisualizerRenderer renderer_;
    ImageRenderer image_renderer_;

    VisualizerRenderConfig render_cfg_;
    VisualizerConfig fft_cfg_;

    uint32_t width_{500};
    uint32_t height_{100};
};

} // namespace obsmv

