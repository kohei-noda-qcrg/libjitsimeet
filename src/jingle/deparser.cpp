#include "common.hpp"
#include "jingle.hpp"

namespace jingle {
namespace {
auto deparse_group(const Jingle::Group& group) -> xml::Node {
    auto node = xml::Node{
        .name = "group",
    }
                    .append_attrs({
                        {"xmlns", ns::grouping},
                        {"semantics", group_semantics_str.find(group.semantics)->second},
                    });
    for(const auto& content : group.contents) {
        node.append_children({
            xml::Node{.name = "content", .attrs = {{"name", content}}},
        });
    }
    return node;
}

auto deparse_rtcp_fb(const Jingle::Content::RTPDescription::PayloadType::RTCPFeedBack& rtcp_fb) -> xml::Node {
    auto node = xml::Node{
        .name = "rtcp-fb",
    }
                    .append_attrs({
                        {"xmlns", ns::rtp_rtcp_fb},
                        {"type", rtcp_fb.type},
                    });
    if(!rtcp_fb.subtype.empty()) {
        node.append_attrs({{"subtype", rtcp_fb.subtype}});
    }
    return node;
}

template <bool optional_value>
auto deparse_parameter(const Parameter<optional_value>& param) -> xml::Node {
    auto node = xml::Node{
        .name = "parameter",
    }
                    .append_attrs({
                        {"name", param.name},
                    });
    if(!param.value.empty()) {
        node.append_attrs({{"value", param.value}});
    }
    return node;
}

auto deparse_payload_type(const Jingle::Content::RTPDescription::PayloadType& pt) -> xml::Node {
    auto node = xml::Node{
        .name = "payload-type",
    }
                    .append_attrs({
                        {"id", std::to_string(pt.id)},
                    });
    if(pt.clockrate != -1) {
        node.append_attrs({{"clockrate", std::to_string(pt.clockrate)}});
    }
    if(pt.channels != -1) {
        node.append_attrs({{"channels", std::to_string(pt.channels)}});
    }
    if(!pt.name.empty()) {
        node.append_attrs({{"name", pt.name}});
    }
    for(const auto& rtcp_fb : pt.rtcp_fbs) {
        node.append_children({deparse_rtcp_fb(rtcp_fb)});
    }
    for(const auto& param : pt.parameters) {
        node.append_children({deparse_parameter(param)});
    }
    return node;
}

auto deparse_source(const Jingle::Content::RTPDescription::Source& src) -> xml::Node {
    auto node = xml::Node{
        .name = "source",
    }
                    .append_attrs({
                        {"xmlns", ns::rtp_ssma},
                        {"ssrc", std::to_string(src.ssrc)},
                        {"name", src.name},
                    });
    if(!src.owner.empty()) {
        node.append_children({
            xml::Node{.name = "ssrc-info", .attrs = {
                                               {"xmlns", ns::jitsi_jitmeet},
                                               {"owner", src.owner},
                                           }},
        });
    }
    for(const auto& param : src.parameters) {
        node.append_children({deparse_parameter(param)});
    }
    return node;
}

auto deparse_rtp_header_ext(const Jingle::Content::RTPDescription::RTPHeaderExt& ext) -> xml::Node {
    auto node = xml::Node{
        .name = "rtp-hdrext",
    }
                    .append_attrs({
                        {"xmlns", ns::rtp_headerext},
                        {"id", std::to_string(ext.id)},
                        {"uri", ext.uri},
                    });
    return node;
}

auto deparse_ssrc_group(const Jingle::Content::RTPDescription::SSRCGroup& ssrc_group) -> xml::Node {
    auto node = xml::Node{
        .name = "ssrc-group",
    }
                    .append_attrs({
                        {"xmlns", ns::rtp_ssma},
                        {"semantics", ssrc_group_semantics_str.find(ssrc_group.semantics)->second},
                    });
    for(const auto ssrc : ssrc_group.ssrcs) {
        node.append_children({
            xml::Node{.name = "source", .attrs = {
                                            {"ssrc", std::to_string(ssrc)},
                                        }},
        });
    }
    return node;
}

auto deparse_rtp_description(const Jingle::Content::RTPDescription& desc) -> xml::Node {
    auto node = xml::Node{
        .name = "description",
    }
                    .append_attrs({
                        {"xmlns", ns::rtp},
                    });
    if(!desc.media.empty()) {
        node.append_attrs({{"media", desc.media}});
    }
    if(desc.ssrc != uint32_t(-1)) {
        node.append_attrs({{"ssrc", std::to_string(desc.ssrc)}});
    }
    for(const auto& pt : desc.payload_types) {
        node.append_children({deparse_payload_type(pt)});
    }
    for(const auto& src : desc.sources) {
        node.append_children({deparse_source(src)});
    }
    for(const auto& ext : desc.rtp_header_exts) {
        node.append_children({deparse_rtp_header_ext(ext)});
    }
    for(const auto& ssrc_group : desc.ssrc_groups) {
        node.append_children({deparse_ssrc_group(ssrc_group)});
    }
    if(desc.support_mux) {
        node.append_children({xml::Node{.name = "rtcp-mux"}});
    }
    return node;
}

auto deparse_fingerprint(const Jingle::Content::IceUdpTransport::FingerPrint& fingerprint) -> xml::Node {
    auto node = xml::Node{
        .name = "fingerprint",
        .data = fingerprint.hash,
    }
                    .append_attrs({
                        {"xmlns", ns::dtls},
                        {"hash", fingerprint.hash_type},
                        {"setup", fingerprint.setup},
                        {"required", fingerprint.required ? "true" : "false"},
                    });
    return node;
}

auto deparse_candidate(const Jingle::Content::IceUdpTransport::Candidate& candidate) -> xml::Node {
    auto node = xml::Node{
        .name = "candidate",
    }
                    .append_attrs({
                        {"component", std::to_string(candidate.component)},
                        {"foundation", candidate.foundation},
                        {"generation", std::to_string(candidate.generation)},
                        {"id", candidate.id},
                        {"ip", candidate.ip_addr},
                        {"port", std::to_string(candidate.port)},
                        {"priority", std::to_string(candidate.priority)},
                        {"protocol", "udp"},
                        {"type", candidate_type_str.find(candidate.type)->second},
                    });
    return node;
}

auto deparse_ice_udp_transport(const Jingle::Content::IceUdpTransport& transport) -> xml::Node {
    auto node = xml::Node{
        .name = "transport",
    }
                    .append_attrs({
                        {"xmlns", ns::transport_ice_udp},
                        {"pwd", transport.pwd},
                        {"ufrag", transport.ufrag},
                    });
    if(transport.support_mux) {
        node.append_children({xml::Node{.name = "rtcp-mux"}});
    }
    for(const auto& candidate : transport.candidates) {
        node.append_children({deparse_candidate(candidate)});
    }
    for(const auto& fingerprint : transport.fingerprints) {
        node.append_children({deparse_fingerprint(fingerprint)});
    }
    if(!transport.websocket.empty()) {
        node.append_children({xml::Node{.name = "web-socket"}.append_attrs({
            {"xmlns", ns::jitsi_colibri},
            {"url", transport.websocket},
        })});
    }
    return node;
}

auto deparse_content(const Jingle::Content& content) -> xml::Node {
    auto node = xml::Node{
        .name = "content",
    }
                    .append_attrs({
                        {"name", content.name},
                        {"senders", content_senders_str.find(content.senders)->second},
                        {"creator", content.is_from_initiator ? "initiator" : "responder"},
                    });
    for(const auto& desc : content.descriptions) {
        node.append_children({deparse_rtp_description(desc)});
    }
    for(const auto& transport : content.transports) {
        node.append_children({deparse_ice_udp_transport(transport)});
    }
    return node;
}
} // namespace

auto deparse(const Jingle& jingle) -> xml::Node {
    auto node = xml::Node{
        .name = "jingle",
    }
                    .append_attrs({
                        {"xmlns", ns::jingle},
                        {"action", action_str.find(jingle.action)->second},
                        {"sid", jingle.sid},
                    });
    if(!jingle.initiator.empty()) {
        node.append_attrs({{"initiator", jingle.initiator}});
    }
    if(!jingle.responder.empty()) {
        node.append_attrs({{"responder", jingle.responder}});
    }
    for(const auto& content : jingle.contents) {
        node.append_children({deparse_content(content)});
    }
    if(jingle.group) {
        node.append_children({deparse_group(*jingle.group)});
    }
    return node;
}
} // namespace jingle
