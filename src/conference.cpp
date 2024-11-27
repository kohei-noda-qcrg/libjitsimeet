#include "conference.hpp"
#include "caps.hpp"
#include "config.hpp"
#include "crypto/base64.hpp"
#include "crypto/sha.hpp"
#include "jingle/jingle.hpp"
#include "macros/unwrap.hpp"
#include "random.hpp"
#include "util/pair-table.hpp"
#include "util/split.hpp"
#include "xmpp/elements.hpp"
#include "json/json.hpp"

namespace conference {
namespace {
auto to_span(const std::string_view str) -> std::span<std::byte> {
    return std::span<std::byte>(std::bit_cast<std::byte*>(str.data()), str.size());
}

auto replace(std::string str, const std::string_view from, const std::string_view to) -> std::string {
    auto pos = size_t(0);

loop:
    pos = str.find(from, pos);
    if(pos == std::string::npos) {
        return str;
    }
    str.replace(pos, from.size(), to);
    pos += to.size();
    goto loop;
}

const auto xml_escape_table = std::array{
    std::pair("&", "&amp;"),
    std::pair("<", "&lt;"),
    std::pair(">", "&gt;"),
    std::pair("\"", "&quot;"),
    std::pair("'", "&apos;"),
};

[[maybe_unused]] auto xml_escape(std::string str) -> std::string {
    for(auto i = xml_escape_table.begin(); i < xml_escape_table.end(); i += 1) {
        str = replace(str, i->first, i->second);
    }
    return str;
}

auto xml_unescape(std::string str) -> std::string {
    for(auto i = xml_escape_table.rbegin(); i < xml_escape_table.rend(); i += 1) {
        str = replace(str, i->second, i->first);
    }
    return str;
}

constexpr auto disco_node = "https://github.com/mojyack/libjitsimeet";
const auto     disco_info = xmpp::elm::query.clone()
                            .append_children({
                                xmpp::elm::identity.clone()
                                    .append_attrs({
                                        {"category", "client"},
                                        {"name", "libjitsimeet"},
                                        {"type", "bot"},
                                        {"xml:lang", "en"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "http://jabber.org/protocol/disco#info"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "urn:xmpp:jingle:apps:rtp:video"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "urn:xmpp:jingle:apps:rtp:audio"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "urn:xmpp:jingle:transports:ice-udp:1"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "urn:xmpp:jingle:apps:dtls:0"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "urn:ietf:rfc:5888"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "urn:ietf:rfc:5761"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "urn:ietf:rfc:4588"},
                                    }),
                                xmpp::elm::feature.clone()
                                    .append_attrs({
                                        {"var", "http://jitsi.org/tcc"},
                                    }),
                            });

const auto codec_type_str = make_pair_table<CodecType, std::string_view>({
    // {CodecType::Opus, "opus"},
    {CodecType::H264, "h264"},
    {CodecType::Vp8, "vp8"},
    {CodecType::Vp9, "vp9"},
});

auto jid_node_to_muc_resource(const std::string_view node) -> std::string {
    for(const auto elm : split(node, "-")) {
        if(!elm.empty()) {
            return std::string(elm);
        }
    }
    return std::string(node);
}

auto handle_iq_get(Conference* const conf, const xml::Node& iq) -> bool {
    unwrap(from, iq.find_attr("from"));
    unwrap(id, iq.find_attr("id"));
    unwrap(query, iq.find_first_child("query"));
    auto iqr = xmpp::elm::iq.clone()
                   .append_attrs({
                       {"from", conf->config.jid.as_full()},
                       {"to", std::string(from)},
                       {"id", std::string(id)},
                       {"type", "result"},
                   });
    const auto node = query.find_attr("node");
    if(node.has_value()) {
        const auto sep = node->rfind("#");
        if(sep == std::string::npos) {
            return false;
        }
        const auto uri  = node->substr(0, sep);
        const auto hash = node->substr(sep + 1);
        if(uri != disco_node || hash != conf->disco_sha1_base64) {
            return false;
        }

        iqr.append_children({
            disco_info.clone()
                .append_attrs({
                    {"node", std::string(*node)},
                }),
        });
    } else {
        iqr.append_children({
            disco_info,
        });
    }
    conf->callbacks->send_payload(xml::deparse(iqr));
    return true;
}

auto handle_iq_set(Conference* const conf, const xml::Node& iq) -> bool {
    unwrap(from, iq.find_attr("from"));
    unwrap(from_jid, xmpp::Jid::parse(from));
    if(from_jid.resource != "focus") {
        return false;
    }
    unwrap(id, iq.find_attr("id"));
    unwrap(jingle_node, iq.find_first_child("jingle"));
    unwrap_mut(jingle, jingle::parse(jingle_node));

    if(config::debug_conference) {
        line_assert("jingle action ", int(jingle.action));
    }
    switch(jingle.action) {
    case jingle::Jingle::Action::SessionInitiate:
        conf->callbacks->on_jingle_initiate(std::move(jingle));
        goto ack;
    case jingle::Jingle::Action::SourceAdd:
        conf->callbacks->on_jingle_add_source(std::move(jingle));
        goto ack;
    default:
        line_warn("unimplemented jingle action: ", int(jingle.action));
        return true;
    }

ack:
    const auto iqr = xmpp::elm::iq.clone()
                         .append_attrs({
                             {"from", conf->config.jid.as_full()},
                             {"to", std::string(from)},
                             {"id", std::string(id)},
                             {"type", "result"},
                         });
    conf->callbacks->send_payload(xml::deparse(iqr));
    return true;
}

auto handle_iq_result(Conference* const conf, const xml::Node& iq, bool success) -> bool {
    unwrap(id, iq.find_attr("id"));
    for(auto i = conf->sent_iqs.begin(); i != conf->sent_iqs.end(); i += 1) {
        if(i->id != id) {
            continue;
        }
        if(!success) {
            line_warn("iq ", id, " failed");
        }
        if(i->on_result) {
            i->on_result(success);
        }
        conf->sent_iqs.erase(i);
        return true;
    }
    bail("stray iq result");
}

auto handle_iq(Conference* const conf, const xml::Node& iq) -> bool {
    unwrap(type, iq.find_attr("type"));
    if(type == "get") {
        return handle_iq_get(conf, iq);
    } else if(type == "set") {
        return handle_iq_set(conf, iq);
    } else if(type == "result") {
        return handle_iq_result(conf, iq, true);
    } else if(type == "error") {
        return handle_iq_result(conf, iq, false);
    }
    return false;
}

auto handle_presence(Conference* const conf, const xml::Node& presence) -> bool {
    const auto response = xml::parse(conf->worker_arg).value();

    unwrap(from_str, presence.find_attr("from"));
    unwrap(from, xmpp::Jid::parse(from_str));
    if(config::debug_conference) {
        line_print("got presence from ", from_str);
    }
    if(const auto type = presence.find_attr("type"); type) {
        if(*type == "unavailable") {
            if(const auto i = conf->participants.find(from.resource); i != conf->participants.end()) {
                conf->callbacks->on_participant_left(i->second);
                conf->participants.erase(i);
            } else {
                line_warn("got unavailable presence from unknown participant");
            }
        }
        return true;
    }

    auto participant = (Participant*)(nullptr);
    auto joined      = false;
    if(const auto i = conf->participants.find(from.resource); i == conf->participants.end()) {
        const auto& p = conf->participants.insert({from.resource,
                                                   Participant{
                                                       .participant_id = from.resource,
                                                       .nick           = "",
                                                       .audio_muted    = true,
                                                       .video_muted    = true,
                                                   }});
        participant   = &p.first->second;
        joined        = true;
    } else {
        participant = &i->second;
    }

    auto audio_muted = std::optional<bool>();
    auto video_muted = std::optional<bool>();

    // note: jitsi web apps seemed to only accept "{video,audio}muted" attribute,
    // but they use "SourceInfo" attribute in their presence.
    // libjitsimeet only uses "{video,audio}muted" in its presence.
    // so we have to handle both.

    for(const auto& payload : presence.children) {
        if(payload.name == xmpp::elm::nick.name && payload.is_attr_equal("xmlns", xmpp::ns::nick)) {
            participant->nick = payload.data;
        } else if(payload.name == "audiomuted" || payload.name == "videomuted") {
            auto muted = bool();
            if(payload.data == "true") {
                muted = true;
            } else if(payload.data == "false") {
                muted = false;
            } else {
                line_warn("unknown {audio,video}muted data: ", payload.data);
            }
            (payload.name == "audiomuted" ? audio_muted : video_muted).emplace(muted);
        } else if(payload.name == "SourceInfo") {
            const auto info_r = json::parse(xml_unescape(payload.data));
            dynamic_assert("failed to parse SourceInfo");
            const auto& info = info_r.value();
            for(auto i = info.children.begin(); i != info.children.end(); i = std::next(i)) {
                const auto& [key, value] = *i;
                const auto object        = value.get<json::Object>();
                if(!object) {
                    continue;
                }
                const auto muted = object->find("muted");
                if(!muted) {
                    continue;
                }
                const auto v = muted->get<json::Boolean>();
                if(!v) {
                    continue;
                }
                const auto& source_name = key;
                if(source_name == participant->participant_id + "-a0") {
                    audio_muted.emplace(v->value);
                } else if(source_name == participant->participant_id + "-v0") {
                    video_muted.emplace(v->value);
                } else {
                    line_warn("unsupported source name format: ", source_name);
                    continue;
                }
            }
        }
    }

    if(joined) {
        conf->callbacks->on_participant_joined(*participant);
    }
    if(audio_muted.has_value()) {
        const auto muted = *audio_muted;
        if(participant->audio_muted != muted) {
            conf->callbacks->on_mute_state_changed(*participant, true, muted);
            participant->audio_muted = muted;
        }
    }
    if(video_muted.has_value()) {
        const auto muted = *video_muted;
        if(participant->video_muted != muted) {
            conf->callbacks->on_mute_state_changed(*participant, false, muted);
            participant->video_muted = muted;
        }
    }

    return true;
}

auto handle_received(Conference* const conf) -> Conference::Worker::Generator {
    // disco
    {
        const auto id   = conf->generate_iq_id();
        const auto muid = build_string("muid_", rng::generate_random_uint32());
        const auto iq   = xmpp::elm::iq.clone()
                            .append_attrs({
                                {"to", conf->config.get_focus_jid().as_full()},
                                {"id", id},
                                {"type", "set"},
                            })
                            .append_children({
                                xmpp::elm::conference.clone()
                                    .append_attrs({
                                        {"machine-uid", muid},
                                        {"room", conf->config.get_muc_jid().as_bare()},
                                    })
                                    .append_children({
                                        xmpp::elm::property.clone()
                                            .append_attrs({
                                                {"stereo", "false"},
                                            }),
                                        xmpp::elm::property.clone()
                                            .append_attrs({
                                                {"startBitrate", "800"},
                                            }),
                                    }),
                            });
        conf->callbacks->send_payload(xml::deparse(iq));
        co_yield true;

        const auto response = xml::parse(conf->worker_arg).value();
        dynamic_assert(response.name == "iq", "unexpected response");
        dynamic_assert(response.is_attr_equal("id", id), "unexpected iq");
        dynamic_assert(response.is_attr_equal("type", "result"), "unexpected iq");
        const auto conference = response.find_first_child("conference");
        dynamic_assert(conference != nullptr);
        dynamic_assert(conference->is_attr_equal("ready", "true"), "conference not ready");
    }
    // presence
    {
        const auto codec_type = codec_type_str.find(conf->config.video_codec_type);
        dynamic_assert(codec_type != nullptr, "invalid codec type config");
        const auto presence =
            xmpp::elm::presence.clone()
                .append_attrs({
                    {"to", conf->config.get_muc_local_jid().as_full()},
                })
                .append_children({
                    xmpp::elm::muc,
                    xmpp::elm::caps.clone()
                        .append_attrs({
                            {"hash", "sha-1"},
                            {"node", disco_node},
                            {"ver", conf->disco_sha1_base64},
                        }),
                    xmpp::elm::ecaps2.clone()
                        .append_children({
                            xmpp::elm::hash.clone()
                                .set_data(conf->disco_sha256_base64)
                                .append_attrs({
                                    {"algo", "sha-256"},
                                }),
                        }),
                    xml::Node{
                        .name = "stats-id",
                        .data = "libjitsimeet",
                    },
                    xml::Node{
                        .name = "jitsi_participant_codecType",
                        .data = codec_type->data(),
                    },
                    xml::Node{
                        .name = "videomuted",
                        .data = conf->config.video_muted ? "true" : "false",
                    },
                    xml::Node{
                        .name = "audiomuted",
                        .data = conf->config.audio_muted ? "true" : "false",
                    },
                    xmpp::elm::nick.clone()
                        .set_data(conf->config.nick),
                });

        conf->callbacks->send_payload(xml::deparse(presence));
        co_yield true;
    }

    // idle
loop:
    auto yield = false;
    do {
        const auto response_r = xml::parse(conf->worker_arg);
        if(!response_r) {
            line_warn("xml parse error");
            break;
        }
        const auto& response = response_r.value();
        if(response.name == "iq") {
            yield = handle_iq(conf, response);
        } else if(response.name == "presence") {
            yield = handle_presence(conf, response);
        } else {
            line_warn("not implemented xmpp message ", response.name);
        }
    } while(0);
    co_yield yield;
    goto loop;
}
} // namespace

auto Config::get_focus_jid() const -> xmpp::Jid {
    return xmpp::Jid{
        .node     = "focus",
        .domain   = std::string("auth.") + jid.domain,
        .resource = "focus",
    };
}

auto Config::get_muc_jid() const -> xmpp::Jid {
    return xmpp::Jid{
        .node     = room,
        .domain   = build_string("conference.") + jid.domain,
        .resource = "",
    };
}

auto Config::get_muc_local_jid() const -> xmpp::Jid {
    auto muc_jid     = get_muc_jid();
    muc_jid.resource = jid_node_to_muc_resource(jid.node);
    return muc_jid;
}

auto Config::get_muc_local_focus_jid() const -> xmpp::Jid {
    auto muc_jid     = get_muc_jid();
    muc_jid.resource = "focus";
    return muc_jid;
}

auto Conference::generate_iq_id() -> std::string {
    return build_string("iq_", (iq_serial += 1));
}

auto Conference::start_negotiation() -> void {
    worker.start(handle_received, this);
    worker.resume();
}

auto Conference::feed_payload(const std::string_view payload) -> bool {
    worker_arg = payload;
    worker.resume();
    return worker.done();
}

auto Conference::send_iq(xml::Node node, std::function<void(bool)> on_result) -> void {
    const auto id = generate_iq_id();
    node.append_attrs({{"id", id}});
    sent_iqs.push_back(SentIq{
        .id        = id,
        .on_result = on_result,
    });
    callbacks->send_payload(xml::deparse(node));
}

auto Conference::create(Config config, ConferenceCallbacks* const callbacks) -> std::unique_ptr<Conference> {
    auto conf = new Conference{
        .config    = std::move(config),
        .callbacks = callbacks,
    };

    const auto disco_str      = compute_disco_str(disco_info);
    const auto disco_sha1     = crypto::sha::calc_sha1(to_span(disco_str));
    const auto disco_sha256   = crypto::sha::calc_sha256(to_span(disco_str));
    conf->disco_sha1_base64   = crypto::base64::encode(disco_sha1);
    conf->disco_sha256_base64 = crypto::base64::encode(disco_sha256);

    return std::unique_ptr<Conference>(conf);
}
} // namespace conference
