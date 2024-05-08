#include "../array-util.hpp"
#include "../assert.hpp"
#include "../util/charconv.hpp"
#include "../xml/xml.hpp"
#include "common.hpp"
#include "jingle.hpp"

namespace jingle {
namespace {
template <class T>
auto parse_template(const xml::Node& node) -> std::optional<T> {
    auto r     = T{};
    auto found = false;

    for(const auto& a : node.attrs) {
        if(0) {
            found = true;
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(0) {
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

#define num_or_nullopt(field, value)                           \
    if(const auto v = from_chars<decltype(field)>(value); v) { \
        field = *v;                                            \
    } else {                                                   \
        return std::nullopt;                                   \
    }

#define assert_ns(field, expect)            \
    if(field != expect) {                   \
        PRINT("unsupported xmlns ", field); \
        return std::nullopt;                \
    }

#define unwrap_parsed_or_nullopt(opt, field) \
    if(const auto p = opt; !p) {             \
        PRINT("parse failed");               \
    } else {                                 \
        field.push_back(*p);                 \
    }

#define search_str_array_or_null(arr, field, value)    \
    if(const auto e = arr.find(value); e != nullptr) { \
        field = e->first;                              \
    } else {                                           \
        PRINT("unknown enum ", value);                 \
        return std::nullopt;                           \
    }

template <bool optional_value>
auto parse_parameter(const xml::Node& node) -> std::optional<Parameter<optional_value>> {
    auto r           = Parameter<optional_value>{};
    auto found_name  = false;
    auto found_value = false;

    for(const auto& a : node.attrs) {
        if(a.key == "name") {
            r.name     = a.value;
            found_name = true;
        } else if(a.key == "value") {
            r.value     = a.value;
            found_value = true;
        } else if(a.key == "xmlns") {
            // TODO
            // check ns
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_name || (!optional_value && !found_value)) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(0) {
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_rtcp_fb(const xml::Node& node) -> std::optional<Jingle::Content::RTPDescription::PayloadType::RTCPFeedBack> {
    auto r          = Jingle::Content::RTPDescription::PayloadType::RTCPFeedBack{};
    auto found_type = false;

    for(const auto& a : node.attrs) {
        if(a.key == "type") {
            found_type = true;
            r.type     = a.value;
        } else if(a.key == "subtype") {
            r.subtype = a.value;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::rtp_rtcp_fb);
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_type) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(0) {
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_payload_type(const xml::Node& node) -> std::optional<Jingle::Content::RTPDescription::PayloadType> {
    auto r        = Jingle::Content::RTPDescription::PayloadType{};
    auto found_id = 0;

    for(const auto& a : node.attrs) {
        if(a.key == "id") {
            num_or_nullopt(r.id, a.value);
            found_id = true;
        } else if(a.key == "clockrate") {
            num_or_nullopt(r.clockrate, a.value);
        } else if(a.key == "channels") {
            num_or_nullopt(r.channels, a.value);
        } else if(a.key == "name") {
            r.name = a.value;
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_id) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(c.name == "rtcp-fb") {
            unwrap_parsed_or_nullopt(parse_rtcp_fb(c), r.rtcp_fbs);
        } else if(c.name == "parameter") {
            unwrap_parsed_or_nullopt(parse_parameter<false>(c), r.parameters);
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_source(const xml::Node& node) -> std::optional<Jingle::Content::RTPDescription::Source> {
    auto r          = Jingle::Content::RTPDescription::Source{};
    auto found_ssrc = false;

    for(const auto& a : node.attrs) {
        if(a.key == "ssrc") {
            num_or_nullopt(r.ssrc, a.value);
            found_ssrc = true;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::rtp_ssma);
        } else if(a.key == "name") {
            // ignore
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_ssrc) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    auto found_owner = false;
    for(const auto& c : node.children) {
        if(c.name == "parameter") {
            unwrap_parsed_or_nullopt(parse_parameter<true>(c), r.parameters);
        } else if(c.name == "ssrc-info") {
            if(!c.is_attr_equal("xmlns", ns::jitsi_jitmeet)) {
                PRINT("invalid ssrc-info");
                return std::nullopt;
            }
            if(const auto owner_o = c.find_attr("owner"); owner_o) {
                r.owner     = *owner_o;
                found_owner = true;
            } else {
                PRINT("ssrc-info has no owner");
                return std::nullopt;
            }
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    if(!found_owner) {
        PRINT("required children not found");
        return std::nullopt;
    }
    return r;
}

auto parse_rtp_header_ext(const xml::Node& node) -> std::optional<Jingle::Content::RTPDescription::RTPHeaderExt> {
    auto r         = Jingle::Content::RTPDescription::RTPHeaderExt{};
    auto found_id  = false;
    auto found_uri = false;

    for(const auto& a : node.attrs) {
        if(a.key == "id") {
            num_or_nullopt(r.id, a.value);
            found_id = true;
        } else if(a.key == "uri") {
            r.uri     = a.value;
            found_uri = true;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::rtp_headerext);
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_id || !found_uri) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(0) {
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_ssrc_group(const xml::Node& node) -> std::optional<Jingle::Content::RTPDescription::SSRCGroup> {
    auto r         = Jingle::Content::RTPDescription::SSRCGroup{};
    auto found_semantics = false;

    for(const auto& a : node.attrs) {
        if(a.key == "semantics") {
            search_str_array_or_null(ssrc_group_semantics_str, r.semantics, a.value);
            found_semantics = true;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::rtp_ssma);
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_semantics) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(c.name == "source") {
            const auto attr = c.find_attr("ssrc");
            if(!attr.has_value()) {
                PRINT("source has not ssrc attribute");
                return std::nullopt;
            }
            auto ssrc = uint32_t(0);
            num_or_nullopt(ssrc, *attr);
            r.ssrcs.push_back(ssrc);
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_rtp_description(const xml::Node& node) -> std::optional<Jingle::Content::RTPDescription> {
    auto r = Jingle::Content::RTPDescription{};

    for(const auto& a : node.attrs) {
        if(a.key == "media") {
            r.media = a.value;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::rtp);
        } else if(a.key == "ssrc") {
            num_or_nullopt(r.ssrc, a.value);
        } else if(a.key == "maxptime") {
            // TODO
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    for(const auto& c : node.children) {
        if(c.name == "payload-type") {
            unwrap_parsed_or_nullopt(parse_payload_type(c), r.payload_types);
        } else if(c.name == "source") {
            unwrap_parsed_or_nullopt(parse_source(c), r.sources);
        } else if(c.name == "rtp-hdrext") {
            unwrap_parsed_or_nullopt(parse_rtp_header_ext(c), r.rtp_header_exts);
        } else if(c.name == "ssrc-group") {
            unwrap_parsed_or_nullopt(parse_ssrc_group(c), r.ssrc_groups);
        } else if(c.name == "rtcp-mux") {
            r.support_mux = true;
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_fingerprint(const xml::Node& node) -> std::optional<Jingle::Content::IceUdpTransport::FingerPrint> {
    if(node.data.empty()) {
        PRINT("empty fingerprint");
        return std::nullopt;
    }

    auto r = Jingle::Content::IceUdpTransport::FingerPrint{
        .hash = std::string(node.data),
    };
    auto found_hash  = false;
    auto found_setup = false;

    for(const auto& a : node.attrs) {
        if(a.key == "hash") {
            r.hash_type = a.value;
            found_hash  = true;
        } else if(a.key == "setup") {
            r.setup     = a.value;
            found_setup = true;
        } else if(a.key == "required") {
            if(a.value == "true") {
                r.required = true;
            } else if(a.value == "false") {
                r.required = false;
            } else {
                PRINT("invalid required");
                return std::nullopt;
            }
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::dtls);
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_hash || !found_setup) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(0) {
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_candidate(const xml::Node& node) -> std::optional<Jingle::Content::IceUdpTransport::Candidate> {
    auto r                = Jingle::Content::IceUdpTransport::Candidate{};
    auto found_component  = false;
    auto found_generation = false;
    auto found_port       = false;
    auto found_priority   = false;
    auto found_type       = false;
    auto found_foundation = false;
    auto found_id         = false;
    auto found_ip_addr    = false;

    for(const auto& a : node.attrs) {
        if(a.key == "component") {
            num_or_nullopt(r.component, a.value);
            found_component = true;
        } else if(a.key == "generation") {
            num_or_nullopt(r.generation, a.value);
            found_generation = true;
        } else if(a.key == "port") {
            num_or_nullopt(r.port, a.value);
            found_port = true;
        } else if(a.key == "priority") {
            num_or_nullopt(r.priority, a.value);
            found_priority = true;
        } else if(a.key == "type") {
            if(const auto e = candidate_type_str.find(a.value); e != nullptr) {
                r.type = e->first;
            } else {
                return std::nullopt;
            }
            search_str_array_or_null(candidate_type_str, r.type, a.value);
            found_type = true;
        } else if(a.key == "foundation") {
            r.foundation     = a.value;
            found_foundation = true;
        } else if(a.key == "id") {
            r.id     = a.value;
            found_id = true;
        } else if(a.key == "ip") {
            r.ip_addr     = a.value;
            found_ip_addr = true;
        } else if(a.key == "protocol") {
            if(a.value != "udp") {
                PRINT("unsupported protocol");
                return std::nullopt;
            }
        } else if(a.key == "network") {
            // ignore
        } else if(a.key == "rel-addr") {
            // ignore
        } else if(a.key == "rel-port") {
            // ignore
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_component || !found_generation || !found_port || !found_priority || !found_type || !found_foundation || !found_id || !found_ip_addr) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(0) {
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_ice_udp_transport(const xml::Node& node) -> std::optional<Jingle::Content::IceUdpTransport> {
    auto r           = Jingle::Content::IceUdpTransport{};
    auto found_pwd   = false;
    auto found_ufrag = false;

    for(const auto& a : node.attrs) {
        if(a.key == "pwd") {
            r.pwd     = a.value;
            found_pwd = true;
        } else if(a.key == "ufrag") {
            r.ufrag     = a.value;
            found_ufrag = true;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::transport_ice_udp);
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_pwd || !found_ufrag) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    auto found_websocket = false;
    for(const auto& c : node.children) {
        if(c.name == "web-socket") {
            if(!c.is_attr_equal("xmlns", ns::jitsi_colibri)) {
                continue;
            }
            if(const auto url = c.find_attr("url"); url) {
                r.websocket     = *url;
                found_websocket = true;
            }
        } else if(c.name == "rtcp-mux") {
            r.support_mux = true;
        } else if(c.name == "fingerprint") {
            unwrap_parsed_or_nullopt(parse_fingerprint(c), r.fingerprints);
        } else if(c.name == "candidate") {
            unwrap_parsed_or_nullopt(parse_candidate(c), r.candidates);
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    if(!found_websocket) {
        PRINT("required children not found");
        return std::nullopt;
    }
    return r;
}

auto parse_content(const xml::Node& node) -> std::optional<Jingle::Content> {
    auto r             = Jingle::Content{};
    auto found_name    = false;

    for(const auto& a : node.attrs) {
        if(a.key == "name") {
            r.name     = a.value;
            found_name = true;
        } else if(a.key == "senders") {
            search_str_array_or_null(content_senders_str, r.senders, a.value);
        } else if(a.key == "creator") {
            if(a.value == "initiator") {
                r.is_from_initiator = true;
            } else if(a.value == "responder") {
                r.is_from_initiator = false;
            } else {
                PRINT("unknown creator ", a.value);
                return std::nullopt;
            }
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_name) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(c.name == "description") {
            if(const auto xmlns = c.find_attr("xmlns"); xmlns) {
                if(*xmlns == ns::rtp) {
                    unwrap_parsed_or_nullopt(parse_rtp_description(c), r.descriptions);
                } else {
                    PRINT("unknown cowntent type ", *xmlns);
                }
            } else {
                PRINT("no xmlns ", *xmlns);
            }
        } else if(c.name == "transport") {
            if(const auto xmlns = c.find_attr("xmlns"); !xmlns) {
                PRINT("no xmlns in transport");
            } else if(*xmlns != ns::transport_ice_udp) {
                PRINT("unsupported transport", *xmlns);
            } else {
                unwrap_parsed_or_nullopt(parse_ice_udp_transport(c), r.transports);
            }
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}

auto parse_group(const xml::Node& node) -> std::optional<Jingle::Group> {
    auto r               = Jingle::Group{};
    auto found_semantics = false;

    for(const auto& a : node.attrs) {
        if(a.key == "semantics") {
            search_str_array_or_null(group_semantics_str, r.semantics, a.value);
            found_semantics = true;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::grouping);
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_semantics) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(c.name == "content") {
            if(const auto name = c.find_attr("name"); name) {
                r.contents.emplace_back(*name);
            }
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}
} // namespace

auto parse(const xml::Node& node) -> std::optional<Jingle> {
    auto r            = Jingle{};
    auto found_action = false;
    auto found_sid    = false;

    for(const auto& a : node.attrs) {
        if(a.key == "action") {
            search_str_array_or_null(action_str, r.action, a.value);
            found_action = true;
        } else if(a.key == "sid") {
            r.sid     = a.value;
            found_sid = true;
        } else if(a.key == "initiator") {
            r.initiator = a.value;
        } else if(a.key == "responder") {
            r.responder = a.value;
        } else if(a.key == "xmlns") {
            assert_ns(a.value, ns::jingle);
        } else {
            PRINT("unhandled attribute ", a.key);
        }
    }
    if(!found_action || !found_sid) {
        PRINT("required attributes not found");
        return std::nullopt;
    }
    for(const auto& c : node.children) {
        if(c.name == "content") {
            unwrap_parsed_or_nullopt(parse_content(c), r.contents);
        } else if(c.name == "group") {
            if(const auto p = parse_group(c); !p) {
                PRINT("parse failed");
            } else {
                r.group.reset(new Jingle::Group{*p});
            }
        } else if(c.name == "bridge-session") {
            // ignore
        } else {
            PRINT("unhandled child ", c.name);
        }
    }
    return r;
}
} // namespace jingle

