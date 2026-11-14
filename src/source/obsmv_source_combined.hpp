#pragma once

#include <obs-module.h>
#include "../visualizer/audio_capture.hpp"
#include "../visualizer/fft_processor.hpp"
#include "../visualizer/visualizer_renderer.hpp"
#include "../render/card_renderer.hpp"
#include <mutex>

namespace obsmv {

enum class CombinedLayout {
    VisualizerTop = 0,
    VisualizerBottom,
    VisualizerLeft,
    VisualizerRight
};

class SourceCombined {
public:
    SourceCombined(obs_data_t* settings, obs_source_t* source);
    ~SourceCombined();

    void update(obs_data_t* settings);
    void tick(float seconds);
    void render(gs_effect_t* effect);

    uint32_t get_width() const;
    uint32_t get_height() const;

    static void register_source();

private:
    obs_source_t* source_{nullptr};
    std::mutex mutex_;

    // Subsystems
    AudioCapture audio_capture_;
    FFTProcessor fft_processor_;
    VisualizerRenderer visualizer_renderer_;
    CardRenderer card_renderer_;

    // Config
    bool show_visualizer_{true};
    bool show_card_{true};
    CombinedLayout layout_{CombinedLayout::VisualizerTop};
    int layout_gap_{16};

    VisualizerRenderConfig viz_render_cfg_;
    VisualizerConfig viz_fft_cfg_;
    CardConfig card_cfg_;

    uint32_t total_width_{500};
    uint32_t total_height_{220};
};

} // namespace obsmv

