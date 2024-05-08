#include "conference.hpp"
#include "util/event.hpp"
#include "util/print.hpp"
#include "websocket.hpp"
#include "xmpp/connection.hpp"

struct ConferenceCallbacks : public conference::ConferenceCallbacks {
    ws::Connection* ws_conn;

    virtual auto send_payload(std::string_view payload) -> void override {
        ws::send_str(ws_conn, payload);
    }

    virtual auto on_jingle_initiate(jingle::Jingle jingle) -> bool override {
        return true;
    }

    virtual auto on_jingle_add_source(jingle::Jingle jingle) -> bool override {
        return true;
    }

    virtual auto on_participant_joined(const conference::Participant& participant) -> void override {
        print("partitipant joined ", participant.participant_id, " ", participant.nick);
    }

    virtual auto on_participant_left(const conference::Participant& participant) -> void override {
        print("partitipant left ", participant.participant_id, " ", participant.nick);
    }
};

auto main() -> int {
    constexpr auto host    = "jitsi.local";
    constexpr auto room    = "room";
    const auto     ws_conn = ws::connect(host, (std::string("xmpp-websocket?room=") + room).data());

    auto ws_tx = [ws_conn](const std::string_view str) {
        ws::send_str(ws_conn, str);
    };

    auto event = Event();
    auto jid   = xmpp::Jid();

    // connet to server
    {
        const auto xmpp_conn = xmpp::create(host, ws_tx);
        ws::add_rx(ws_conn, [xmpp_conn, &event](const std::span<std::byte> data) -> ws::RxResult {
            const auto done = xmpp::resume_negotiation(xmpp_conn, std::string_view((char*)data.data(), data.size()));
            if(done) {
                event.wakeup();
                return ws::RxResult::Complete;
            }
            return ws::RxResult::Handled;
        });
        xmpp::start_negotiation(xmpp_conn);
        event.wait();
        jid = xmpp::finish(xmpp_conn).jid;
    }

    event.clear();

    // join conference
    {
        auto callbacks        = ConferenceCallbacks();
        callbacks.ws_conn     = ws_conn;
        const auto conference = conference::Conference::create(room, jid, &callbacks);

        ws::add_rx(ws_conn, [&conference, &event](const std::span<std::byte> data) -> ws::RxResult {
            const auto done = conference->feed_payload(std::string_view((char*)data.data(), data.size()));
            if(done) {
                event.wakeup();
                return ws::RxResult::Complete;
            }
            return ws::RxResult::Handled;
        });
        conference->start_negotiation();
        event.wait();
    }
    return 0;
}
