#pragma once

#include <obs-module.h>
#include "../render/card_renderer.hpp"
#include <mutex>

namespace obsmv {

class SourceNowPlaying {
public:
    SourceNowPlaying(obs_data_t* settings, obs_source_t* source);
    ~SourceNowPlaying();

    void update(obs_data_t* settings);
    void tick(float seconds);
    void render(gs_effect_t* effect);

    uint32_t get_width() const { return static_cast<uint32_t>(card_cfg_.width); }
    uint32_t get_height() const { return static_cast<uint32_t>(card_cfg_.height); }

    static void register_source();

private:
    obs_source_t* source_{nullptr};
    std::mutex mutex_;

    CardRenderer card_renderer_;
    CardConfig card_cfg_;
};

} // namespace obsmv

