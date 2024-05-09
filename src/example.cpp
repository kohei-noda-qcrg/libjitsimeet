#include "conference.hpp"
#include "jingle-handler/jingle.hpp"
#include "util/event.hpp"
#include "util/print.hpp"
#include "websocket.hpp"
#include "xmpp/elements.hpp"
#include "xmpp/negotiator.hpp"

struct XMPPNegotiatorCallbacks : public xmpp::NegotiatorCallbacks {
    ws::Connection* ws_conn;

    virtual auto send_payload(std::string_view payload) -> void override {
        ws::send_str(ws_conn, payload);
    }
};

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

struct Args {
    const char* host   = nullptr;
    const char* room   = nullptr;
    bool        secure = true;

    static auto parse(const int argc, const char* const argv[]) -> Args {
        auto args = Args();
        for(auto i = 1; i < argc; i += 1) {
            if(std::strcmp(argv[i], "-s") == 0) {
                args.secure = false;
            } else {
                if(args.host == nullptr) {
                    args.host = argv[i];
                } else if(args.room == nullptr) {
                    args.room = argv[i];
                } else {
                    warn("too many arguments");
                    exit(1);
                }
            }
        }
        return args;
    }
};

auto main(const int argc, const char* const argv[]) -> int {
    if(argc < 3) {
        print("usage: example [-s] HOST ROOM");
        print("    -s: allow self-signed cert");
        return 1;
    }
    const auto args    = Args::parse(argc, argv);
    const auto ws_path = std::string("xmpp-websocket?room=") + args.room;
    const auto ws_conn = ws::create_connection(args.host, ws_path.data(), args.secure);

    auto event  = Event();
    auto jid    = xmpp::Jid();
    auto ext_sv = std::vector<xmpp::Service>();

    // gain jid from server
    {
        auto callbacks        = XMPPNegotiatorCallbacks();
        callbacks.ws_conn     = ws_conn;
        const auto negotiator = xmpp::Negotiator::create(args.host, &callbacks);
        ws::add_receiver(ws_conn, [&negotiator, &event](const std::span<std::byte> data) -> ws::ReceiverResult {
            const auto payload = std::string_view(std::bit_cast<char*>(data.data()), data.size());
            const auto done    = negotiator->feed_payload(payload);
            if(done) {
                event.wakeup();
                return ws::ReceiverResult::Complete;
            } else {
                return ws::ReceiverResult::Handled;
            }
        });
        negotiator->start_negotiation();
        event.wait();

        jid    = std::move(negotiator->jid);
        ext_sv = std::move(negotiator->external_services);
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
        const auto conference    = conference::Conference::create(args.room, jid, &callbacks);
        ws::add_receiver(ws_conn, [&conference](const std::span<std::byte> data) -> ws::ReceiverResult {
            // feed_payload always returns true in current implementation
            conference->feed_payload(std::string_view((char*)data.data(), data.size()));
            return ws::ReceiverResult::Handled;
        });
        conference->start_negotiation();
        event.wait();
        conference->send_jingle_accept(jingle_handler.build_accept_jingle().value());

        auto count = 0;
        while(true) {
            const auto id = build_string("ping_", count += 1);
            const auto iq = xmpp::elm::iq.clone()
                                .append_attrs({
                                    {"id", id},
                                    {"type", "get"},
                                })
                                .append_children({
                                    xmpp::elm::ping,
                                });
            ws::send_str(ws_conn, xml::deparse(iq));
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    return 0;
}
