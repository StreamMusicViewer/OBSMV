#include "obsmv_source_combined.hpp"
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

SourceCombined::SourceCombined(obs_data_t* settings, obs_source_t* source)
    : source_(source), fft_processor_(1024) {
    update(settings);
}

SourceCombined::~SourceCombined() {}

void SourceCombined::update(obs_data_t* settings) {
    std::lock_guard<std::mutex> lock(mutex_);

    show_visualizer_ = obs_data_get_bool(settings, "show_visualizer");
    show_card_ = obs_data_get_bool(settings, "show_card");
    layout_ = static_cast<CombinedLayout>(obs_data_get_int(settings, "layout"));
    layout_gap_ = static_cast<int>(obs_data_get_int(settings, "layout_gap"));

    // Audio capture & FFT
    std::string audio_src = obs_data_get_string(settings, "audio_source");
    audio_capture_.set_source(audio_src);

    viz_fft_cfg_.bar_count = static_cast<int>(obs_data_get_int(settings, "bar_count"));
    viz_fft_cfg_.scale_boost = static_cast<float>(obs_data_get_double(settings, "scale_boost"));
    viz_fft_cfg_.gravity = static_cast<float>(obs_data_get_double(settings, "gravity"));
    viz_fft_cfg_.smoothing = static_cast<VisualizerSmoothing>(obs_data_get_int(settings, "smoothing"));
    fft_processor_.configure(viz_fft_cfg_);

    // Visualizer renderer
    viz_render_cfg_.style = static_cast<VisualizerStyle>(obs_data_get_int(settings, "viz_style"));
    viz_render_cfg_.bar_count = viz_fft_cfg_.bar_count;
    viz_render_cfg_.bar_width = static_cast<int>(obs_data_get_int(settings, "bar_width"));
    viz_render_cfg_.bar_gap = static_cast<int>(obs_data_get_int(settings, "bar_gap"));
    viz_render_cfg_.max_height = static_cast<int>(obs_data_get_int(settings, "viz_height"));
    viz_render_cfg_.color_top = static_cast<uint32_t>(obs_data_get_int(settings, "color_top"));
    viz_render_cfg_.color_bottom = static_cast<uint32_t>(obs_data_get_int(settings, "color_bottom"));
    viz_render_cfg_.dynamic_color = obs_data_get_bool(settings, "dynamic_color");
    viz_render_cfg_.dynamic_influence = static_cast<float>(obs_data_get_double(settings, "dynamic_influence"));
    viz_render_cfg_.opacity = static_cast<float>(obs_data_get_double(settings, "viz_opacity"));
    viz_render_cfg_.wave_crest = obs_data_get_bool(settings, "wave_crest");

    // Card config
    card_cfg_.width = static_cast<int>(obs_data_get_int(settings, "card_width"));
    card_cfg_.height = static_cast<int>(obs_data_get_int(settings, "card_height"));
    card_cfg_.padding = static_cast<int>(obs_data_get_int(settings, "card_padding"));
    card_cfg_.art_size = static_cast<int>(obs_data_get_int(settings, "art_size"));
    card_cfg_.spin_art = obs_data_get_bool(settings, "spin_art");
    card_cfg_.show_background = obs_data_get_bool(settings, "show_background");
    card_cfg_.bg_color = static_cast<uint32_t>(obs_data_get_int(settings, "bg_color"));
    card_cfg_.card_opacity = static_cast<float>(obs_data_get_double(settings, "card_opacity"));
    card_cfg_.border_color = static_cast<uint32_t>(obs_data_get_int(settings, "border_color"));
    card_cfg_.border_width = static_cast<int>(obs_data_get_int(settings, "border_width"));

    card_cfg_.title_size = static_cast<float>(obs_data_get_double(settings, "title_size"));
    card_cfg_.artist_size = static_cast<float>(obs_data_get_double(settings, "artist_size"));
    card_cfg_.album_size = static_cast<float>(obs_data_get_double(settings, "album_size"));
    card_cfg_.title_color = static_cast<uint32_t>(obs_data_get_int(settings, "title_color"));
    card_cfg_.artist_color = static_cast<uint32_t>(obs_data_get_int(settings, "artist_color"));
    card_cfg_.album_color = static_cast<uint32_t>(obs_data_get_int(settings, "album_color"));
    card_cfg_.title_format = obs_data_get_string(settings, "title_format");
    card_cfg_.artist_format = obs_data_get_string(settings, "artist_format");
    card_cfg_.show_album = obs_data_get_bool(settings, "show_album");

    card_cfg_.show_progress = obs_data_get_bool(settings, "show_progress");
    card_cfg_.progress_height = static_cast<int>(obs_data_get_int(settings, "progress_height"));
    card_cfg_.progress_color = static_cast<uint32_t>(obs_data_get_int(settings, "progress_color"));
    card_cfg_.dynamic_color = viz_render_cfg_.dynamic_color;
    card_cfg_.dynamic_influence = viz_render_cfg_.dynamic_influence;
    card_cfg_.theme_mode = static_cast<CardThemeMode>(obs_data_get_int(settings, "theme_mode"));
    card_cfg_.font_path = obs_data_get_string(settings, "custom_font_path");

    // Read native OBS system font selector
    obs_data_t* font_obj = obs_data_get_obj(settings, "font");
    if (font_obj) {
        const char* face = obs_data_get_string(font_obj, "face");
        if (face && strlen(face) > 0) {
            card_cfg_.font_face = face;
        }
        obs_data_release(font_obj);
    }

    card_renderer_.configure(card_cfg_);

    auto media_mode = static_cast<MediaSourceMode>(obs_data_get_int(settings, "media_mode"));
    MediaManager::instance().set_mode(media_mode);
    MediaManager::instance().set_osmv_json_path(obs_data_get_string(settings, "osmv_json_path"));

    // Dimensions
    if (show_visualizer_ && show_card_) {
        if (layout_ == CombinedLayout::VisualizerTop || layout_ == CombinedLayout::VisualizerBottom) {
            total_width_ = card_cfg_.width;
            total_height_ = viz_render_cfg_.max_height + layout_gap_ + card_cfg_.height;
        } else {
            total_width_ = card_cfg_.width + layout_gap_ + card_cfg_.width;
            total_height_ = std::max(viz_render_cfg_.max_height, card_cfg_.height);
        }
    } else if (show_visualizer_) {
        total_width_ = card_cfg_.width;
        total_height_ = viz_render_cfg_.max_height;
    } else {
        total_width_ = card_cfg_.width;
        total_height_ = card_cfg_.height;
    }
}

void SourceCombined::tick(float seconds) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Reconnect audio source if needed
    audio_capture_.tick(seconds);

    // 2. Fetch song info
    SongInfo song = MediaManager::instance().get_current_song();
    card_renderer_.update(song, seconds);

    // 3. Capture audio & compute FFT with smooth decay
    if (show_visualizer_) {
        std::vector<float> left_pcm, right_pcm;
        bool has_audio = audio_capture_.fetch_samples(left_pcm, right_pcm, 1024);
        fft_processor_.process(left_pcm, right_pcm, audio_capture_.get_sample_rate(), seconds, has_audio);
    }
}

void SourceCombined::render(gs_effect_t*) {
    std::lock_guard<std::mutex> lock(mutex_);

    uint32_t accent = card_renderer_.get_accent_color();
    uint32_t primary = card_renderer_.get_primary_color();

    float viz_x = 0.0f, viz_y = 0.0f;
    float card_x = 0.0f, card_y = 0.0f;

    if (show_visualizer_ && show_card_) {
        if (layout_ == CombinedLayout::VisualizerTop) {
            viz_x = 0.0f;
            viz_y = 0.0f;
            card_x = 0.0f;
            card_y = viz_render_cfg_.max_height + layout_gap_;
        } else if (layout_ == CombinedLayout::VisualizerBottom) {
            card_x = 0.0f;
            card_y = 0.0f;
            viz_x = 0.0f;
            viz_y = card_cfg_.height + layout_gap_;
        }
    }

    if (show_visualizer_) {
        visualizer_renderer_.render(fft_processor_.get_left_bins(),
                                    fft_processor_.get_right_bins(),
                                    viz_render_cfg_,
                                    accent,
                                    primary,
                                    viz_x, viz_y,
                                    static_cast<float>(card_cfg_.width),
                                    static_cast<float>(viz_render_cfg_.max_height));
    }

    if (show_card_) {
        card_renderer_.render(card_x, card_y);
    }
}

uint32_t SourceCombined::get_width() const { return total_width_; }
uint32_t SourceCombined::get_height() const { return total_height_; }

static obs_properties_t* get_properties_cb(void*) {
    obs_properties_t* props = obs_properties_create();

    obs_properties_add_bool(props, "show_visualizer", tr("ShowVisualizer", "Afficher le visualiseur audio"));
    obs_properties_add_bool(props, "show_card", tr("ShowCard", "Afficher la carte de musique"));

    obs_property_t* layout = obs_properties_add_list(props, "layout", tr("Layout", "Disposition des elements"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(layout, tr("LayoutVizTop", "Visualiseur en haut / Musique en bas"), static_cast<int>(CombinedLayout::VisualizerTop));
    obs_property_list_add_int(layout, tr("LayoutVizBottom", "Musique en haut / Visualiseur en bas"), static_cast<int>(CombinedLayout::VisualizerBottom));

    obs_properties_add_int(props, "layout_gap", tr("LayoutGap", "Espacement entre elements (px)"), 0, 100, 1);

    obs_property_t* src_list = obs_properties_add_list(props, "audio_source", tr("AudioSource", "Source audio"),
                                                       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(src_list, tr("None", "Aucune"), "none");
    obs_enum_sources(add_audio_source_cb, src_list);

    // Visualizer Settings Group
    obs_properties_t* viz_group = obs_properties_create();
    obs_property_t* v_style = obs_properties_add_list(viz_group, "viz_style", tr("VizStyle", "Style de visualiseur"),
                                                      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(v_style, tr("Bars", "Barres verticales"), static_cast<int>(VisualizerStyle::Bars));
    obs_property_list_add_int(v_style, tr("Mirrored", "Barres symetriques / centrees"), static_cast<int>(VisualizerStyle::Mirrored));
    obs_property_list_add_int(v_style, tr("Wave", "Onde / Oscillogramme"), static_cast<int>(VisualizerStyle::Wave));
    obs_property_list_add_int(v_style, tr("Circular", "Circulaire / Radial"), static_cast<int>(VisualizerStyle::Circular));

    obs_properties_add_int(viz_group, "bar_count", tr("BarCount", "Nombre de barres"), 8, 128, 1);
    obs_properties_add_int(viz_group, "bar_width", tr("BarWidth", "Largeur des barres"), 1, 50, 1);
    obs_properties_add_int(viz_group, "bar_gap", tr("BarGap", "Espacement des barres"), 0, 30, 1);
    obs_properties_add_int(viz_group, "viz_height", tr("VizHeight", "Hauteur du visualiseur"), 20, 500, 1);
    obs_properties_add_float_slider(viz_group, "scale_boost", tr("ScaleBoost", "Sensibilite / Boost"), 0.1, 5.0, 0.1);
    obs_properties_add_float_slider(viz_group, "gravity", tr("Gravity", "Gravite / Retombee"), 0.1, 1.0, 0.05);

    obs_property_t* smooth = obs_properties_add_list(viz_group, "smoothing", tr("Smoothing", "Filtre de lissage"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(smooth, "Monstercat", static_cast<int>(VisualizerSmoothing::Monstercat));
    obs_property_list_add_int(smooth, "Gravity", static_cast<int>(VisualizerSmoothing::Gravity));
    obs_property_list_add_int(smooth, "None", static_cast<int>(VisualizerSmoothing::None));

    obs_properties_add_color_alpha(viz_group, "color_top", tr("ColorTop", "Couleur haute / Accent"));
    obs_properties_add_color_alpha(viz_group, "color_bottom", tr("ColorBottom", "Couleur basse"));
    obs_properties_add_float_slider(viz_group, "viz_opacity", tr("VizOpacity", "Opacite du visualiseur"), 0.0, 1.0, 0.05);
    obs_properties_add_bool(viz_group, "dynamic_color", tr("DynamicColor", "Couleur dynamique (Sync pochette)"));
    obs_properties_add_float_slider(viz_group, "dynamic_influence", tr("DynamicInfluence", "Influence couleur dynamique (Style Photoshop)"), 0.0, 1.0, 0.05);
    obs_properties_add_bool(viz_group, "wave_crest", tr("WaveCrest", "Ligne de crete lumineuse (Contour de l'onde)"));

    obs_properties_add_group(props, "viz_settings", tr("VisualizerSettings", "Parametres du visualiseur"),
                             OBS_GROUP_NORMAL, viz_group);

    // Music Card Settings Group
    obs_properties_t* card_group = obs_properties_create();

    obs_property_t* t_mode = obs_properties_add_list(card_group, "theme_mode", tr("ThemeMode", "Mode de theme"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(t_mode, tr("ThemeAuto", "Automatique (Adapte a la pochette)"), static_cast<int>(CardThemeMode::Auto));
    obs_property_list_add_int(t_mode, tr("ThemeDark", "Sombre / Dark Glass"), static_cast<int>(CardThemeMode::Dark));
    obs_property_list_add_int(t_mode, tr("ThemeLight", "Clair / Frosted Light"), static_cast<int>(CardThemeMode::Light));

    obs_properties_add_font(card_group, "font", tr("Font", "Police d'ecriture du systeme"));

    obs_property_t* m_mode = obs_properties_add_list(card_group, "media_mode", tr("MediaMode", "Source des medias"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(m_mode, tr("MediaModeAuto", "Auto-detection (SMTC/MPRIS + OSMV Sync)"), static_cast<int>(MediaSourceMode::Auto));
    obs_property_list_add_int(m_mode, tr("MediaModeNative", "OS natif uniquement (SMTC / MPRIS)"), static_cast<int>(MediaSourceMode::NativeOnly));
    obs_property_list_add_int(m_mode, tr("MediaModeOsmv", "Synchronisation OSMV daemon (current_song.json)"), static_cast<int>(MediaSourceMode::OsmvSync));

    obs_properties_add_path(card_group, "osmv_json_path", tr("OsmvJsonPath", "Chemin vers current_song.json"),
                            OBS_PATH_FILE, "JSON (*.json)", nullptr);

    obs_properties_add_int(card_group, "card_width", tr("CardWidth", "Largeur de la carte"), 200, 1920, 10);
    obs_properties_add_int(card_group, "card_height", tr("CardHeight", "Hauteur de la carte"), 60, 1080, 5);
    obs_properties_add_int(card_group, "card_padding", tr("CardPadding", "Marge interieure (Padding)"), 0, 50, 1);
    obs_properties_add_int(card_group, "art_size", tr("ArtSize", "Taille de la pochette"), 20, 300, 2);
    obs_properties_add_bool(card_group, "spin_art", tr("SpinArt", "Rotation de la pochette en lecture"));

    obs_properties_add_bool(card_group, "show_background", tr("ShowBackground", "Afficher le fond de la carte"));
    obs_properties_add_color_alpha(card_group, "bg_color", tr("BgColor", "Couleur d'arriere-plan"));
    obs_properties_add_float_slider(card_group, "card_opacity", tr("CardOpacity", "Opacite du fond de carte"), 0.0, 1.0, 0.05);
    obs_properties_add_color_alpha(card_group, "border_color", tr("BorderColor", "Couleur de bordure"));
    obs_properties_add_int(card_group, "border_width", tr("BorderWidth", "Epaisseur de bordure"), 0, 10, 1);

    obs_properties_add_float_slider(card_group, "title_size", tr("TitleSize", "Taille police Titre"), 10.0, 60.0, 1.0);
    obs_properties_add_color_alpha(card_group, "title_color", tr("TitleColor", "Couleur du titre"));
    obs_properties_add_float_slider(card_group, "artist_size", tr("ArtistSize", "Taille police Artiste"), 8.0, 40.0, 1.0);
    obs_properties_add_color_alpha(card_group, "artist_color", tr("ArtistColor", "Couleur de l'artiste"));
    obs_properties_add_text(card_group, "title_format", tr("TitleFormat", "Format Titre"), OBS_TEXT_DEFAULT);
    obs_properties_add_text(card_group, "artist_format", tr("ArtistFormat", "Format Artiste"), OBS_TEXT_DEFAULT);

    obs_properties_add_bool(card_group, "show_album", tr("ShowAlbum", "Afficher l'album"));
    obs_properties_add_float_slider(card_group, "album_size", tr("AlbumSize", "Taille police Album"), 8.0, 40.0, 1.0);
    obs_properties_add_color_alpha(card_group, "album_color", tr("AlbumColor", "Couleur de l'album"));

    obs_properties_add_bool(card_group, "show_progress", tr("ShowProgress", "Afficher la barre de progression"));
    obs_properties_add_int(card_group, "progress_height", tr("ProgressHeight", "Hauteur barre de progression"), 1, 20, 1);
    obs_properties_add_color_alpha(card_group, "progress_color", tr("ProgressColor", "Couleur de progression"));

    obs_properties_add_path(card_group, "custom_font_path", tr("CustomFont", "Police personnalisee (.ttf/.otf)"),
                            OBS_PATH_FILE, "Font (*.ttf;*.otf)", nullptr);

    obs_properties_add_group(props, "card_settings", tr("CardSettings", "Parametres de la carte musicale"),
                             OBS_GROUP_NORMAL, card_group);

    return props;
}

static void get_defaults_cb(obs_data_t* settings) {
    obs_data_set_default_bool(settings, "show_visualizer", true);
    obs_data_set_default_bool(settings, "show_card", true);
    obs_data_set_default_int(settings, "layout", static_cast<int>(CombinedLayout::VisualizerTop));
    obs_data_set_default_int(settings, "layout_gap", 16);

    obs_data_set_default_string(settings, "audio_source", "none");
    obs_data_set_default_int(settings, "viz_style", static_cast<int>(VisualizerStyle::Bars));
    obs_data_set_default_int(settings, "bar_count", 32);
    obs_data_set_default_int(settings, "bar_width", 8);
    obs_data_set_default_int(settings, "bar_gap", 4);
    obs_data_set_default_int(settings, "viz_height", 80);
    obs_data_set_default_double(settings, "scale_boost", 1.0);
    obs_data_set_default_double(settings, "gravity", 0.85);
    obs_data_set_default_int(settings, "smoothing", static_cast<int>(VisualizerSmoothing::Monstercat));
    obs_data_set_default_int(settings, "color_top", make_obs_color(170, 0, 255, 255));
    obs_data_set_default_int(settings, "color_bottom", make_obs_color(85, 0, 127, 255));
    obs_data_set_default_bool(settings, "dynamic_color", true);
    obs_data_set_default_double(settings, "dynamic_influence", 1.0);
    obs_data_set_default_double(settings, "viz_opacity", 1.0);
    obs_data_set_default_bool(settings, "wave_crest", true);

    obs_data_set_default_int(settings, "theme_mode", static_cast<int>(CardThemeMode::Auto));

    obs_data_t* def_font = obs_data_create();
    obs_data_set_string(def_font, "face", "Segoe UI");
    obs_data_set_int(def_font, "size", 18);
    obs_data_set_default_obj(settings, "font", def_font);
    obs_data_release(def_font);

    obs_data_set_default_int(settings, "media_mode", static_cast<int>(MediaSourceMode::Auto));
    obs_data_set_default_int(settings, "card_width", 450);
    obs_data_set_default_int(settings, "card_height", 110);
    obs_data_set_default_int(settings, "card_padding", 14);
    obs_data_set_default_int(settings, "art_size", 80);
    obs_data_set_default_bool(settings, "spin_art", false);
    obs_data_set_default_bool(settings, "show_background", true);
    obs_data_set_default_int(settings, "bg_color", make_obs_color(18, 18, 20, 230));
    obs_data_set_default_double(settings, "card_opacity", 0.90);
    obs_data_set_default_int(settings, "border_color", make_obs_color(255, 255, 255, 38));
    obs_data_set_default_int(settings, "border_width", 1);

    obs_data_set_default_double(settings, "title_size", 18.0);
    obs_data_set_default_int(settings, "title_color", make_obs_color(255, 255, 255, 255));
    obs_data_set_default_double(settings, "artist_size", 14.0);
    obs_data_set_default_int(settings, "artist_color", make_obs_color(179, 179, 179, 255));
    obs_data_set_default_string(settings, "title_format", "{title}");
    obs_data_set_default_string(settings, "artist_format", "{artist}");
    obs_data_set_default_bool(settings, "show_album", true);
    obs_data_set_default_double(settings, "album_size", 12.0);
    obs_data_set_default_int(settings, "album_color", make_obs_color(138, 155, 191, 255));

    obs_data_set_default_bool(settings, "show_progress", true);
    obs_data_set_default_int(settings, "progress_height", 4);
    obs_data_set_default_int(settings, "progress_color", make_obs_color(170, 0, 255, 255));
}

void SourceCombined::register_source() {
    obs_source_info si{};
    si.id = "obsmv";
    si.type = OBS_SOURCE_TYPE_INPUT;
    si.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    si.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT;

    si.get_name = [](void*) { return tr("SourceNameCombined", "OBSMV (Visualiseur + Musique)"); };
    si.create = [](obs_data_t* settings, obs_source_t* source) {
        return static_cast<void*>(new SourceCombined(settings, source));
    };
    si.destroy = [](void* data) { delete static_cast<SourceCombined*>(data); };
    si.get_width = [](void* data) { return static_cast<SourceCombined*>(data)->get_width(); };
    si.get_height = [](void* data) { return static_cast<SourceCombined*>(data)->get_height(); };
    si.get_properties = get_properties_cb;
    si.get_defaults = get_defaults_cb;
    si.update = [](void* data, obs_data_t* s) { static_cast<SourceCombined*>(data)->update(s); };
    si.video_tick = [](void* data, float sec) { static_cast<SourceCombined*>(data)->tick(sec); };
    si.video_render = [](void* data, gs_effect_t* effect) {
        static_cast<SourceCombined*>(data)->render(effect);
    };

    obs_register_source(&si);
}

} // namespace obsmv
