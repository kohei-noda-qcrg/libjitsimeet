#include <coop/blocker.hpp>
#include <coop/generator.hpp>
#include <coop/parallel.hpp>
#include <coop/promise.hpp>
#include <coop/single-event.hpp>
#include <coop/task-injector.hpp>
#include <coop/timer.hpp>

#include "async-websocket.hpp"
#include "colibri.hpp"
#include "conference.hpp"
#include "jingle-handler/jingle.hpp"
#include "macros/assert.hpp"
#include "util/argument-parser.hpp"
#include "util/assert.hpp"
#include "util/print.hpp"
#include "util/span.hpp"
#include "xmpp/elements.hpp"
#include "xmpp/negotiator.hpp"

namespace {
struct XMPPNegotiatorCallbacks : public xmpp::NegotiatorCallbacks {
    ws::client::Context* ws_context;

    virtual auto send_payload(std::string_view payload) -> void override {
        ensure(ws_context->send(payload));
    }
};

struct ConferenceCallbacks : public conference::ConferenceCallbacks {
    ws::client::Context* ws_context;
    JingleHandler*       jingle_handler;

    virtual auto send_payload(std::string_view payload) -> void override {
        ensure(ws_context->send(payload));
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

    virtual auto on_mute_state_changed(const conference::Participant& participant, const bool is_audio, const bool new_muted) -> void override {
        print("mute state changed ", participant.participant_id, " ", is_audio ? "audio" : "video", "=", new_muted);
    }
};

auto async_main(const int argc, const char* const argv[]) -> coop::Async<int> {
    constexpr auto error_value = -1;

    const char* host   = nullptr;
    const char* room   = nullptr;
    auto        secure = true;
    {
        auto help   = false;
        auto parser = args::Parser<>();
        parser.arg(&host, "HOST", "server domain");
        parser.arg(&room, "ROOM", "room name");
        parser.kwflag(&secure, {"-s"}, "allow self-signed ssl certificate", {.invert_flag_value = true});
        parser.kwflag(&help, {"-h", "--help"}, "print this help message", {.no_error_check = true});
        if(!parser.parse(argc, argv) || help) {
            print("usage: example ", parser.get_help());
            co_return 0;
        }
    }

    auto injector   = coop::TaskInjector(*co_await coop::reveal_runner());
    auto ws_context = ws::client::AsyncContext();
    co_ensure_v(ws_context.init(
        injector,
        {
            .address   = host,
            .path      = build_string("xmpp-websocket?room=", room).data(),
            .protocol  = "xmpp",
            .port      = 443,
            .ssl_level = secure ? ws::client::SSLLevel::Enable : ws::client::SSLLevel::TrustSelfSigned,
        }));

    auto ws_task = coop::TaskHandle();
    co_await coop::run_args(ws_context.process_until_finish()).detach({&ws_task});

    auto event  = coop::SingleEvent();
    auto jid    = xmpp::Jid();
    auto ext_sv = std::vector<xmpp::Service>();

    // gain jid from server
    {
        auto callbacks        = XMPPNegotiatorCallbacks();
        callbacks.ws_context  = &ws_context;
        const auto negotiator = xmpp::Negotiator::create(host, &callbacks);

        ws_context.handler = [&negotiator, &event](const std::span<const std::byte> data) -> coop::Async<void> {
            switch(negotiator->feed_payload(from_span(data))) {
            case xmpp::FeedResult::Continue:
                break;
            case xmpp::FeedResult::Error:
                line_panic();
            case xmpp::FeedResult::Done:
                event.notify();
                break;
            }
            co_return;
        };
        negotiator->start_negotiation();
        co_await event;

        jid    = std::move(negotiator->jid);
        ext_sv = std::move(negotiator->external_services);
    }

    // join conference
    {
        constexpr auto audio_codec_type = CodecType::Opus;
        constexpr auto video_codec_type = CodecType::H264;

        auto jingle_handler      = JingleHandler(audio_codec_type, video_codec_type, jid, ext_sv, &event);
        auto callbacks           = ConferenceCallbacks();
        callbacks.ws_context     = &ws_context;
        callbacks.jingle_handler = &jingle_handler;
        const auto conference    = conference::Conference::create(
            conference::Config{
                   .jid              = jid,
                   .room             = room,
                   .nick             = "libjitsimeet-example",
                   .video_codec_type = video_codec_type,
                   .audio_muted      = false,
                   .video_muted      = false,
            },
            &callbacks);
        ws_context.handler = [&conference](const std::span<const std::byte> data) -> coop::Async<void> {
            conference->feed_payload(from_span(data));
            co_return;
        };
        conference->start_negotiation();
        co_await event;
        {
            auto accept    = jingle_handler.build_accept_jingle().value();
            auto accept_iq = xmpp::elm::iq.clone()
                                 .append_attrs({
                                     {"from", jid.as_full()},
                                     {"to", conference->config.get_muc_local_focus_jid().as_full()},
                                     {"type", "set"},
                                 })
                                 .append_children({
                                     jingle::deparse(accept),
                                 });

            conference->send_iq(std::move(accept_iq), [](bool success) -> void {
                dynamic_assert(success, "failed to send accept iq");
            });
        }

        auto colibri = colibri::Colibri::connect(jingle_handler.get_session().initiate_jingle, secure);
        colibri->set_last_n(5);

        while(true) {
            const auto iq = xmpp::elm::iq.clone()
                                .append_attrs({
                                    {"type", "get"},
                                })
                                .append_children({
                                    xmpp::elm::ping,
                                });
            conference->send_iq(iq, {});
            co_await coop::sleep(std::chrono::seconds(10));
        }
    }
    ws_task.cancel();
}
} // namespace

auto main(const int argc, const char* const argv[]) -> int {
    auto runner = coop::Runner();
    runner.push_task(async_main(argc, argv));
    runner.run();
    return 0;
}
