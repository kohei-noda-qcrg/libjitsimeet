#pragma once
#include <functional>

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

    virtual ~ConferenceCallbacks(){};
};

struct SentIq {
    std::string               id;
    std::function<void(bool)> on_result; // optional
};

struct Conference {
    using Worker = CoRoutine<bool>;

    // constant
    xmpp::Jid            jid;                 // self jid in the server
    xmpp::Jid            focus_jid;           // focus jid in the server
    xmpp::Jid            muc_jid;             // room jid
    xmpp::Jid            muc_local_jid;       // self jid in the room
    xmpp::Jid            muc_local_focus_jid; // focus jid in the room
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

    static auto create(std::string_view room, const xmpp::Jid& jid, ConferenceCallbacks* callbacks) -> std::unique_ptr<Conference>;

    ~Conference(){};
};
} // namespace conference

