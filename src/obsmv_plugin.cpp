#include <obs-module.h>
#include "source/obsmv_source_combined.hpp"
#include "source/obsmv_source_visualizer.hpp"
#include "source/obsmv_source_nowplaying.hpp"
#include "media/media_detector.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obsmv", "en-US")

MODULE_EXPORT const char* obs_module_description(void) {
    return "OBSMV - Native Audio Visualizer & Now Playing Music Widget";
}

MODULE_EXPORT const char* obs_module_name(void) {
    return "OBSMV";
}

bool obs_module_load(void) {
    blog(LOG_INFO, "[OBSMV] Loading plugin...");

    // Start background media monitor thread
    obsmv::MediaManager::instance().start();

    // Register all three sources
    obsmv::SourceCombined::register_source();
    obsmv::SourceVisualizer::register_source();
    obsmv::SourceNowPlaying::register_source();

    blog(LOG_INFO, "[OBSMV] Plugin loaded successfully (Combined, Visualizer, NowPlaying sources registered).");
    return true;
}

void obs_module_unload(void) {
    blog(LOG_INFO, "[OBSMV] Unloading plugin...");
    obsmv::MediaManager::instance().stop();
}

