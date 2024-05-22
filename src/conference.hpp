#pragma once
#include <functional>

#include "codec-type.hpp"
#include "jingle/jingle.hpp"
#include "util/coroutine.hpp"
#include "util/string-map.hpp"
#include "xmpp/jid.hpp"

namespace conference {
struct Participant {
    std::string participant_id;
    std::string nick;
};

struct ConferenceCallbacks {
    virtual auto send_payload(std::string_view /*payload*/) -> void = 0;

    virtual auto on_jingle_initiate(jingle::Jingle /*jingle*/) -> bool {
        return true;
    }

    virtual auto on_jingle_add_source(jingle::Jingle /*jingle*/) -> bool {
        return true;
    }

    virtual auto on_participant_joined(const Participant& /*participant*/) -> void {
    }

    virtual auto on_participant_left(const Participant& /*participant*/) -> void {
    }

    virtual auto on_source_mute_info(const std::string_view /*source_name*/, const bool /*muted*/) -> void {
    }

    virtual ~ConferenceCallbacks(){};
};

struct SentIq {
    std::string               id;
    std::function<void(bool)> on_result; // optional
};

struct Config {
    xmpp::Jid   jid;
    std::string room;
    std::string nick;
    CodecType   video_codec_type;
    bool        audio_muted;
    bool        video_muted;

    auto get_focus_jid() const -> xmpp::Jid;
    auto get_muc_jid() const -> xmpp::Jid;
    auto get_muc_local_jid() const -> xmpp::Jid;
    auto get_muc_local_focus_jid() const -> xmpp::Jid;
};

struct Conference {
    using Worker = CoRoutine<bool>;

    // constant
    Config               config;
    std::string          disco_sha1_base64;
    std::string          disco_sha256_base64;
    ConferenceCallbacks* callbacks;

    // coroutine
    std::string_view worker_arg;
    Worker           worker;

    // state
    std::vector<SentIq>    sent_iqs;
    StringMap<Participant> participants;
    static inline int      iq_serial;

    auto generate_iq_id() -> std::string;
    auto start_negotiation() -> void;
    auto feed_payload(std::string_view payload) -> bool;
    auto send_iq(xml::Node iq, std::function<void(bool)> on_result) -> void;

    static auto create(Config config, ConferenceCallbacks* callbacks) -> std::unique_ptr<Conference>;

    ~Conference(){};
};
} // namespace conference

