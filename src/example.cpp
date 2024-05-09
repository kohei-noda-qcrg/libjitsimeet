#include "conference.hpp"
#include "jingle-handler/jingle.hpp"
#include "util/event.hpp"
#include "util/print.hpp"
#include "websocket.hpp"
#include "xmpp/connection.hpp"

struct ConferenceCallbacks : public conference::ConferenceCallbacks {
    ws::Connection* ws_conn;
    JingleHandler*  jingle_handler;

    virtual auto send_payload(std::string_view payload) -> void override {
        ws::send_str(ws_conn, payload);
    }

    virtual auto on_jingle_initiate(jingle::Jingle jingle) -> bool override {
        return jingle_handler->on_initiate(std::move(jingle));
    }

    virtual auto on_jingle_add_source(jingle::Jingle jingle) -> bool override {
        return jingle_handler->on_add_source(std::move(jingle));
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

    auto event  = Event();
    auto jid    = xmpp::Jid();
    auto ext_sv = std::vector<xmpp::Service>();

    // connet to server
    {
        const auto xmpp_conn = xmpp::create(host, [ws_conn](const std::string_view str) {
            ws::send_str(ws_conn, str);
        });
        ws::add_receiver(ws_conn, [xmpp_conn, &event](const std::span<std::byte> data) -> ws::ReceiverResult {
            const auto done = xmpp::resume_negotiation(xmpp_conn, std::string_view((char*)data.data(), data.size()));
            if(done) {
                event.wakeup();
                return ws::ReceiverResult::Complete;
            }
            return ws::ReceiverResult::Handled;
        });
        xmpp::start_negotiation(xmpp_conn);
        event.wait();

        auto res = xmpp::finish(xmpp_conn);
        jid      = std::move(res.jid);
        ext_sv   = std::move(res.external_services);
    }

    event.clear();

    // join conference
    {
        constexpr auto audio_codec_type = CodecType::Opus;
        constexpr auto video_codec_type = CodecType::H264;

        auto jingle_handler      = JingleHandler(audio_codec_type, video_codec_type, jid, ext_sv, &event);
        auto callbacks           = ConferenceCallbacks();
        callbacks.ws_conn        = ws_conn;
        callbacks.jingle_handler = &jingle_handler;
        const auto conference    = conference::Conference::create(room, jid, &callbacks);

        ws::add_receiver(ws_conn, [&conference, &event](const std::span<std::byte> data) -> ws::ReceiverResult {
            const auto done = conference->feed_payload(std::string_view((char*)data.data(), data.size()));
            if(done) {
                event.wakeup();
                return ws::ReceiverResult::Complete;
            }
            return ws::ReceiverResult::Handled;
        });
        conference->start_negotiation();
        event.wait();
        conference->send_jingle_accept(jingle_handler.build_accept_jingle().value());

        auto count = 1;
        while(true) {
            const auto payload = build_string(R"(<iq xmlns='jabber:client' id=")", "ping_",
                                              count += 1,
                                              R"(" type="get"><ping xmlns='urn:xmpp:ping'/></iq>)");
            ws::send_str(ws_conn, payload);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    return 0;
}
