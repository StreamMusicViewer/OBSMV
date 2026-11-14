#include "media_detector.hpp"

#if defined(__linux__) && !defined(_WIN32) && !defined(__APPLE__)
#include <dbus/dbus.h>
#include <vector>
#include <fstream>
#include <cstring>

namespace obsmv {

namespace {

uint64_t hash_bytes(const uint8_t* data, size_t size) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

std::vector<uint8_t> read_file_bytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

class LinuxMprisProvider final : public IMediaProvider {
public:
    LinuxMprisProvider() {
        DBusError err;
        dbus_error_init(&err);
        conn_ = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (dbus_error_is_set(&err)) {
            dbus_error_free(&err);
            conn_ = nullptr;
        }
    }

    ~LinuxMprisProvider() override {
        if (conn_) {
            dbus_connection_unref(conn_);
            conn_ = nullptr;
        }
    }

    bool poll(SongInfo& out_info) override {
        if (!conn_) return false;

        DBusMessage* msg = dbus_message_new_method_call(
            "org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus",
            "ListNames");
        if (!msg) return false;

        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn_, msg, 1000, &err);
        dbus_message_unref(msg);

        if (!reply || dbus_error_is_set(&err)) {
            dbus_error_free(&err);
            if (reply) dbus_message_unref(reply);
            return false;
        }

        DBusMessageIter iter, sub;
        if (!dbus_message_iter_init(reply, &iter) ||
            dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
            dbus_message_unref(reply);
            return false;
        }

        dbus_message_iter_recurse(&iter, &sub);
        std::string mpris_service;
        while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
            const char* name = nullptr;
            dbus_message_iter_get_basic(&sub, &name);
            if (name && strncmp(name, "org.mpris.MediaPlayer2.", 23) == 0) {
                mpris_service = name;
                break;
            }
            dbus_message_iter_next(&sub);
        }
        dbus_message_unref(reply);

        if (mpris_service.empty()) return false;

        out_info.player = mpris_service.substr(23);

        // Query PlaybackStatus
        DBusMessage* prop_msg = dbus_message_new_method_call(
            mpris_service.c_str(),
            "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties",
            "Get");
        if (prop_msg) {
            const char* iface = "org.mpris.MediaPlayer2.Player";
            const char* prop = "PlaybackStatus";
            dbus_message_append_args(prop_msg,
                                     DBUS_TYPE_STRING, &iface,
                                     DBUS_TYPE_STRING, &prop,
                                     DBUS_TYPE_INVALID);
            DBusMessage* prop_reply = dbus_connection_send_with_reply_and_block(conn_, prop_msg, 500, &err);
            dbus_message_unref(prop_msg);
            if (prop_reply && !dbus_error_is_set(&err)) {
                DBusMessageIter v_iter, val_iter;
                if (dbus_message_iter_init(prop_reply, &v_iter) &&
                    dbus_message_iter_get_arg_type(&v_iter) == DBUS_TYPE_VARIANT) {
                    dbus_message_iter_recurse(&v_iter, &val_iter);
                    if (dbus_message_iter_get_arg_type(&val_iter) == DBUS_TYPE_STRING) {
                        const char* status = nullptr;
                        dbus_message_iter_get_basic(&val_iter, &status);
                        if (status) {
                            out_info.status = (strcmp(status, "Playing") == 0) ? "playing" :
                                              (strcmp(status, "Paused") == 0) ? "paused" : "stopped";
                            out_info.is_playing = (out_info.status == "playing");
                        }
                    }
                }
                dbus_message_unref(prop_reply);
            }
            dbus_error_free(&err);
        }

        return !out_info.title.empty();
    }

private:
    DBusConnection* conn_ = nullptr;
};

} // namespace

std::unique_ptr<IMediaProvider> create_native_media_provider() {
    return std::make_unique<LinuxMprisProvider>();
}

} // namespace obsmv
#endif

