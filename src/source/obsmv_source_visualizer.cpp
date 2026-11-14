#include "obsmv_source_visualizer.hpp"
#include "../media/media_detector.hpp"
#include "../render/color_utils.hpp"
#include <cstring>

namespace obsmv {

static const char* tr(const char* key, const char* fallback) {
    const char* str = obs_module_text(key);
    if (!str || strcmp(str, key) == 0) {
        return fallback;
    }
    return str;
}

static bool add_audio_source_cb(void* param, obs_source_t* source) {
    auto* list = static_cast<obs_property_t*>(param);
    uint32_t flags = obs_source_get_output_flags(source);
    if ((flags & OBS_SOURCE_AUDIO) != 0) {
        const char* name = obs_source_get_name(source);
        obs_property_list_add_string(list, name, name);
    }
    return true;
}

SourceVisualizer::SourceVisualizer(obs_data_t* settings, obs_source_t* source)
    : source_(source), fft_processor_(1024) {
    update(settings);
}

SourceVisualizer::~SourceVisualizer() {}

void SourceVisualizer::update(obs_data_t* settings) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string audio_src = obs_data_get_string(settings, "audio_source");
    audio_capture_.set_source(audio_src);

    width_ = static_cast<uint32_t>(obs_data_get_int(settings, "width"));
    height_ = static_cast<uint32_t>(obs_data_get_int(settings, "height"));

    fft_cfg_.bar_count = static_cast<int>(obs_data_get_int(settings, "bar_count"));
    fft_cfg_.scale_boost = static_cast<float>(obs_data_get_double(settings, "scale_boost"));
    fft_cfg_.gravity = static_cast<float>(obs_data_get_double(settings, "gravity"));
    fft_cfg_.smoothing = static_cast<VisualizerSmoothing>(obs_data_get_int(settings, "smoothing"));
    fft_processor_.configure(fft_cfg_);

    render_cfg_.style = static_cast<VisualizerStyle>(obs_data_get_int(settings, "viz_style"));
    render_cfg_.bar_count = fft_cfg_.bar_count;
    render_cfg_.bar_width = static_cast<int>(obs_data_get_int(settings, "bar_width"));
    render_cfg_.bar_gap = static_cast<int>(obs_data_get_int(settings, "bar_gap"));
    render_cfg_.max_height = static_cast<int>(height_);
    render_cfg_.color_top = static_cast<uint32_t>(obs_data_get_int(settings, "color_top"));
    render_cfg_.color_bottom = static_cast<uint32_t>(obs_data_get_int(settings, "color_bottom"));
    render_cfg_.dynamic_color = obs_data_get_bool(settings, "dynamic_color");
    render_cfg_.dynamic_influence = static_cast<float>(obs_data_get_double(settings, "dynamic_influence"));
    render_cfg_.opacity = static_cast<float>(obs_data_get_double(settings, "viz_opacity"));
    render_cfg_.wave_crest = obs_data_get_bool(settings, "wave_crest");
}

void SourceVisualizer::tick(float seconds) {
    std::lock_guard<std::mutex> lock(mutex_);

    audio_capture_.tick(seconds);

    std::vector<float> left_pcm, right_pcm;
    bool has_audio = audio_capture_.fetch_samples(left_pcm, right_pcm, 1024);
    fft_processor_.process(left_pcm, right_pcm, audio_capture_.get_sample_rate(), seconds, has_audio);

    if (render_cfg_.dynamic_color) {
        SongInfo song = MediaManager::instance().get_current_song();
        if (song.has_cover && song.cover_hash != 0) {
            image_renderer_.update_image(song.cover_data, song.cover_hash);
        }
    }
}

void SourceVisualizer::render(gs_effect_t*) {
    std::lock_guard<std::mutex> lock(mutex_);

    uint32_t accent = render_cfg_.dynamic_color ? image_renderer_.get_accent_color() : render_cfg_.color_top;
    uint32_t primary = render_cfg_.dynamic_color ? image_renderer_.get_primary_color() : render_cfg_.color_bottom;

    renderer_.render(fft_processor_.get_left_bins(),
                     fft_processor_.get_right_bins(),
                     render_cfg_,
                     accent,
                     primary,
                     0.0f, 0.0f,
                     static_cast<float>(width_),
                     static_cast<float>(height_));
}

static obs_properties_t* get_properties_cb(void*) {
    obs_properties_t* props = obs_properties_create();

    obs_properties_add_int(props, "width", tr("Width", "Largeur"), 50, 3840, 10);
    obs_properties_add_int(props, "height", tr("Height", "Hauteur"), 20, 2160, 5);

    obs_property_t* src_list = obs_properties_add_list(props, "audio_source", tr("AudioSource", "Source audio"),
                                                       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(src_list, tr("None", "Aucune"), "none");
    obs_enum_sources(add_audio_source_cb, src_list);

    obs_property_t* v_style = obs_properties_add_list(props, "viz_style", tr("VizStyle", "Style de visualiseur"),
                                                      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(v_style, tr("Bars", "Barres verticales"), static_cast<int>(VisualizerStyle::Bars));
    obs_property_list_add_int(v_style, tr("Mirrored", "Barres symetriques / centrees"), static_cast<int>(VisualizerStyle::Mirrored));
    obs_property_list_add_int(v_style, tr("Wave", "Onde / Oscillogramme"), static_cast<int>(VisualizerStyle::Wave));
    obs_property_list_add_int(v_style, tr("Circular", "Circulaire / Radial"), static_cast<int>(VisualizerStyle::Circular));

    obs_properties_add_int(props, "bar_count", tr("BarCount", "Nombre de barres"), 8, 128, 1);
    obs_properties_add_int(props, "bar_width", tr("BarWidth", "Largeur des barres"), 1, 50, 1);
    obs_properties_add_int(props, "bar_gap", tr("BarGap", "Espacement des barres"), 0, 30, 1);
    obs_properties_add_float_slider(props, "scale_boost", tr("ScaleBoost", "Sensibilite / Boost"), 0.1, 5.0, 0.1);
    obs_properties_add_float_slider(props, "gravity", tr("Gravity", "Gravite / Retombee"), 0.1, 1.0, 0.05);

    obs_property_t* smooth = obs_properties_add_list(props, "smoothing", tr("Smoothing", "Filtre de lissage"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(smooth, "Monstercat", static_cast<int>(VisualizerSmoothing::Monstercat));
    obs_property_list_add_int(smooth, "Gravity", static_cast<int>(VisualizerSmoothing::Gravity));
    obs_property_list_add_int(smooth, "None", static_cast<int>(VisualizerSmoothing::None));

    obs_properties_add_color_alpha(props, "color_top", tr("ColorTop", "Couleur haute / Accent"));
    obs_properties_add_color_alpha(props, "color_bottom", tr("ColorBottom", "Couleur basse"));
    obs_properties_add_float_slider(props, "viz_opacity", tr("VizOpacity", "Opacite du visualiseur"), 0.0, 1.0, 0.05);
    obs_properties_add_bool(props, "dynamic_color", tr("DynamicColor", "Couleur dynamique (Sync pochette)"));
    obs_properties_add_float_slider(props, "dynamic_influence", tr("DynamicInfluence", "Influence couleur dynamique (Style Photoshop)"), 0.0, 1.0, 0.05);
    obs_properties_add_bool(props, "wave_crest", tr("WaveCrest", "Ligne de crete lumineuse (Contour de l'onde)"));

    return props;
}

static void get_defaults_cb(obs_data_t* settings) {
    obs_data_set_default_int(settings, "width", 500);
    obs_data_set_default_int(settings, "height", 100);
    obs_data_set_default_string(settings, "audio_source", "none");
    obs_data_set_default_int(settings, "viz_style", static_cast<int>(VisualizerStyle::Bars));
    obs_data_set_default_int(settings, "bar_count", 32);
    obs_data_set_default_int(settings, "bar_width", 8);
    obs_data_set_default_int(settings, "bar_gap", 4);
    obs_data_set_default_double(settings, "scale_boost", 1.0);
    obs_data_set_default_double(settings, "gravity", 0.85);
    obs_data_set_default_int(settings, "smoothing", static_cast<int>(VisualizerSmoothing::Monstercat));
    obs_data_set_default_int(settings, "color_top", make_obs_color(170, 0, 255, 255));
    obs_data_set_default_int(settings, "color_bottom", make_obs_color(85, 0, 127, 255));
    obs_data_set_default_double(settings, "viz_opacity", 1.0);
    obs_data_set_default_bool(settings, "dynamic_color", false);
    obs_data_set_default_double(settings, "dynamic_influence", 1.0);
    obs_data_set_default_bool(settings, "wave_crest", true);
}

void SourceVisualizer::register_source() {
    obs_source_info si{};
    si.id = "obsmv_visualizer";
    si.type = OBS_SOURCE_TYPE_INPUT;
    si.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    si.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT;

    si.get_name = [](void*) { return tr("SourceNameVisualizer", "OBSMV Visualiseur"); };
    si.create = [](obs_data_t* settings, obs_source_t* source) {
        return static_cast<void*>(new SourceVisualizer(settings, source));
    };
    si.destroy = [](void* data) { delete static_cast<SourceVisualizer*>(data); };
    si.get_width = [](void* data) { return static_cast<SourceVisualizer*>(data)->get_width(); };
    si.get_height = [](void* data) { return static_cast<SourceVisualizer*>(data)->get_height(); };
    si.get_properties = get_properties_cb;
    si.get_defaults = get_defaults_cb;
    si.update = [](void* data, obs_data_t* s) { static_cast<SourceVisualizer*>(data)->update(s); };
    si.video_tick = [](void* data, float sec) { static_cast<SourceVisualizer*>(data)->tick(sec); };
    si.video_render = [](void* data, gs_effect_t* effect) {
        static_cast<SourceVisualizer*>(data)->render(effect);
    };

    obs_register_source(&si);
}

} // namespace obsmv
