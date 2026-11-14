#include "obsmv_source_nowplaying.hpp"
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

SourceNowPlaying::SourceNowPlaying(obs_data_t* settings, obs_source_t* source)
    : source_(source) {
    update(settings);
}

SourceNowPlaying::~SourceNowPlaying() {}

void SourceNowPlaying::update(obs_data_t* settings) {
    std::lock_guard<std::mutex> lock(mutex_);

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
    card_cfg_.dynamic_color = obs_data_get_bool(settings, "dynamic_color");
    card_cfg_.dynamic_influence = static_cast<float>(obs_data_get_double(settings, "dynamic_influence"));
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
}

void SourceNowPlaying::tick(float seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    SongInfo song = MediaManager::instance().get_current_song();
    card_renderer_.update(song, seconds);
}

void SourceNowPlaying::render(gs_effect_t*) {
    std::lock_guard<std::mutex> lock(mutex_);
    card_renderer_.render(0.0f, 0.0f);
}

static obs_properties_t* get_properties_cb(void*) {
    obs_properties_t* props = obs_properties_create();

    obs_property_t* t_mode = obs_properties_add_list(props, "theme_mode", tr("ThemeMode", "Mode de theme"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(t_mode, tr("ThemeAuto", "Automatique (Adapte a la pochette)"), static_cast<int>(CardThemeMode::Auto));
    obs_property_list_add_int(t_mode, tr("ThemeDark", "Sombre / Dark Glass"), static_cast<int>(CardThemeMode::Dark));
    obs_property_list_add_int(t_mode, tr("ThemeLight", "Clair / Frosted Light"), static_cast<int>(CardThemeMode::Light));

    obs_properties_add_font(props, "font", tr("Font", "Police d'ecriture du systeme"));

    obs_property_t* m_mode = obs_properties_add_list(props, "media_mode", tr("MediaMode", "Source des medias"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(m_mode, tr("MediaModeAuto", "Auto-detection (SMTC/MPRIS + OSMV Sync)"), static_cast<int>(MediaSourceMode::Auto));
    obs_property_list_add_int(m_mode, tr("MediaModeNative", "OS natif uniquement (SMTC / MPRIS)"), static_cast<int>(MediaSourceMode::NativeOnly));
    obs_property_list_add_int(m_mode, tr("MediaModeOsmv", "Synchronisation OSMV daemon (current_song.json)"), static_cast<int>(MediaSourceMode::OsmvSync));

    obs_properties_add_path(props, "osmv_json_path", tr("OsmvJsonPath", "Chemin vers current_song.json"),
                            OBS_PATH_FILE, "JSON (*.json)", nullptr);

    obs_properties_add_int(props, "card_width", tr("CardWidth", "Largeur de la carte"), 200, 1920, 10);
    obs_properties_add_int(props, "card_height", tr("CardHeight", "Hauteur de la carte"), 60, 1080, 5);
    obs_properties_add_int(props, "card_padding", tr("CardPadding", "Marge interieure (Padding)"), 0, 50, 1);
    obs_properties_add_int(props, "art_size", tr("ArtSize", "Taille de la pochette"), 20, 300, 2);
    obs_properties_add_bool(props, "spin_art", tr("SpinArt", "Rotation de la pochette en lecture"));

    obs_properties_add_bool(props, "show_background", tr("ShowBackground", "Afficher le fond de la carte"));
    obs_properties_add_color_alpha(props, "bg_color", tr("BgColor", "Couleur d'arriere-plan"));
    obs_properties_add_float_slider(props, "card_opacity", tr("CardOpacity", "Opacite du fond de carte"), 0.0, 1.0, 0.05);
    obs_properties_add_color_alpha(props, "border_color", tr("BorderColor", "Couleur de bordure"));
    obs_properties_add_int(props, "border_width", tr("BorderWidth", "Epaisseur de bordure"), 0, 10, 1);

    obs_properties_add_float_slider(props, "title_size", tr("TitleSize", "Taille police Titre"), 10.0, 60.0, 1.0);
    obs_properties_add_color_alpha(props, "title_color", tr("TitleColor", "Couleur du titre"));
    obs_properties_add_float_slider(props, "artist_size", tr("ArtistSize", "Taille police Artiste"), 8.0, 40.0, 1.0);
    obs_properties_add_color_alpha(props, "artist_color", tr("ArtistColor", "Couleur de l'artiste"));
    obs_properties_add_text(props, "title_format", tr("TitleFormat", "Format Titre"), OBS_TEXT_DEFAULT);
    obs_properties_add_text(props, "artist_format", tr("ArtistFormat", "Format Artiste"), OBS_TEXT_DEFAULT);

    obs_properties_add_bool(props, "show_album", tr("ShowAlbum", "Afficher l'album"));
    obs_properties_add_float_slider(props, "album_size", tr("AlbumSize", "Taille police Album"), 8.0, 40.0, 1.0);
    obs_properties_add_color_alpha(props, "album_color", tr("AlbumColor", "Couleur de l'album"));

    obs_properties_add_bool(props, "show_progress", tr("ShowProgress", "Afficher la barre de progression"));
    obs_properties_add_int(props, "progress_height", tr("ProgressHeight", "Hauteur barre de progression"), 1, 20, 1);
    obs_properties_add_color_alpha(props, "progress_color", tr("ProgressColor", "Couleur de progression"));
    obs_properties_add_bool(props, "dynamic_color", tr("DynamicColor", "Couleur dynamique (Sync pochette)"));
    obs_properties_add_float_slider(props, "dynamic_influence", tr("DynamicInfluence", "Influence couleur dynamique (Style Photoshop)"), 0.0, 1.0, 0.05);

    obs_properties_add_path(props, "custom_font_path", tr("CustomFont", "Police personnalisee (.ttf/.otf)"),
                            OBS_PATH_FILE, "Font (*.ttf;*.otf)", nullptr);

    return props;
}

static void get_defaults_cb(obs_data_t* settings) {
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
    obs_data_set_default_bool(settings, "dynamic_color", true);
    obs_data_set_default_double(settings, "dynamic_influence", 1.0);
}

void SourceNowPlaying::register_source() {
    obs_source_info si{};
    si.id = "obsmv_nowplaying";
    si.type = OBS_SOURCE_TYPE_INPUT;
    si.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    si.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT;

    si.get_name = [](void*) { return tr("SourceNameNowPlaying", "OBSMV Musique en cours"); };
    si.create = [](obs_data_t* settings, obs_source_t* source) {
        return static_cast<void*>(new SourceNowPlaying(settings, source));
    };
    si.destroy = [](void* data) { delete static_cast<SourceNowPlaying*>(data); };
    si.get_width = [](void* data) { return static_cast<SourceNowPlaying*>(data)->get_width(); };
    si.get_height = [](void* data) { return static_cast<SourceNowPlaying*>(data)->get_height(); };
    si.get_properties = get_properties_cb;
    si.get_defaults = get_defaults_cb;
    si.update = [](void* data, obs_data_t* s) { static_cast<SourceNowPlaying*>(data)->update(s); };
    si.video_tick = [](void* data, float sec) { static_cast<SourceNowPlaying*>(data)->tick(sec); };
    si.video_render = [](void* data, gs_effect_t* effect) {
        static_cast<SourceNowPlaying*>(data)->render(effect);
    };

    obs_register_source(&si);
}

} // namespace obsmv
