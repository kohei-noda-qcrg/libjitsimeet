#pragma once
#include <functional>

#include "jingle-handler.hpp"
#include "util/coroutine.hpp"
#include "util/string-map.hpp"
#include "xmpp/jid.hpp"

namespace conference {
struct Participant {
    std::string participant_id;
    std::string nick;
};

struct Conference {
    using Worker = CoRoutine<bool>;

    // constant
    xmpp::Jid   jid;                 // self jid in the server
    xmpp::Jid   focus_jid;           // focus jid in the server
    xmpp::Jid   muc_jid;             // room jid
    xmpp::Jid   muc_local_jid;       // self jid in the room
    xmpp::Jid   muc_local_focus_jid; // focus jid in the room
    std::string disco_sha1_base64;
    std::string disco_sha256_base64;

    // coroutine
    std::string_view worker_arg;
    Worker           worker;

    std::string            jingle_accept_iq_id;
    StringMap<Participant> participants;
    int                    iq_serial;

    // owner must init these fields
    JingleHandler*                          jingle_handler;
    std::function<void(std::string_view)>   send_payload;
    std::function<void(const Participant&)> on_participant_joined;
    std::function<void(const Participant&)> on_participant_left;

    auto generate_iq_id() -> std::string;
    auto start_negotiation() -> void;
    auto feed_payload(std::string_view payload) -> bool;
    auto send_jingle_accept(jingle::Jingle jingle) -> void;

    static auto create(std::string_view room, const xmpp::Jid& jid) -> std::unique_ptr<Conference>;

    ~Conference(){};
};
} // namespace conference

