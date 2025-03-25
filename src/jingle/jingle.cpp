#include "jingle.hpp"
#include "../util/pair-table.hpp"

#define SERDE_NO_INCLUDE
#include "serde/xml/format.hpp"

#include "serde/macro.hpp"

namespace jingle {
const auto action_str = make_pair_table<Action, std::string_view>({
    {Action::ContentAccept, "content-accept"},
    {Action::ContentAdd, "content-add"},
    {Action::ContentModify, "content-modify"},
    {Action::ContentReject, "content-reject"},
    {Action::ContentRemove, "content-remove"},
    {Action::DescriptionInfo, "description-info"},
    {Action::SecurityInfo, "security-info"},
    {Action::SessionAccept, "session-accept"},
    {Action::SessionInfo, "session-info"},
    {Action::SessionInitiate, "session-initiate"},
    {Action::SessionTerminate, "session-terminate"},
    {Action::TransportAccept, "transport-accept"},
    {Action::TransportInfo, "transport-info"},
    {Action::TransportReject, "transport-reject"},
    {Action::TransportReplace, "transport-replace"},
    {Action::SourceAdd, "source-add"},
    {Action::SourceRemove, "source-remove"},
});

const auto senders_str = make_pair_table<Senders, std::string_view>({
    {Senders::Both, "both"},
    {Senders::Initiator, "initiator"},
    {Senders::Responder, "responder"},
    {Senders::None, "none"},
});

inline const auto ssrc_semantics_str = make_pair_table<SSRCSemantics, std::string_view>({
    {SSRCSemantics::Ls, "LS"},
    {SSRCSemantics::Fid, "FID"},
    {SSRCSemantics::Srf, "SRF"},
    {SSRCSemantics::Anat, "ANAT"},
    {SSRCSemantics::Fec, "FEC"},
    {SSRCSemantics::Ddp, "DDP"},
});

inline const auto candidate_type_str = make_pair_table<CandidateType, std::string_view>({
    {CandidateType::Host, "host"},
    {CandidateType::Prflx, "prflx"},
    {CandidateType::Relay, "relay"},
    {CandidateType::Srflx, "srflx"},
});

inline const auto group_semantics_str = make_pair_table<GroupSemantics, std::string_view>({
    {GroupSemantics::LipSync, "LS"},
    {GroupSemantics::Bundle, "BUNDLE"},
});

} // namespace jingle

namespace serde {
template <serde_struct T>
inline auto serialize(XmlFormat& /*format*/, const char* const name, ::xml::Node& payload, const jingle::XMLNameSpace<T>& /*data*/) -> bool {
    payload[name] = T::ns;
    return true;
}

template <serde_struct T>
inline auto deserialize(XmlFormat& /*format*/, const char* const name, const ::xml::Node& payload, jingle::XMLNameSpace<T>& /*data*/) -> bool {
    unwrap(attr, payload.find_attr(name));
    ensure(attr == T::ns);
    return true;
}

#define enum_conv(Enum, enum)                                                     \
    template <>                                                                   \
    auto to_string(const jingle::Enum data) -> std::optional<std::string> {       \
        if(const auto ptr = jingle::enum##_str.find(data)) {                      \
            return std::string(*ptr);                                             \
        } else {                                                                  \
            return std::nullopt;                                                  \
        }                                                                         \
    }                                                                             \
    template <>                                                                   \
    auto from_string(const std::string_view str) -> std::optional<jingle::Enum> { \
        if(const auto ptr = jingle::enum##_str.find(str)) {                       \
            return *ptr;                                                          \
        } else {                                                                  \
            return std::nullopt;                                                  \
        }                                                                         \
    }

enum_conv(Action, action);
enum_conv(Senders, senders);
enum_conv(SSRCSemantics, ssrc_semantics);
enum_conv(CandidateType, candidate_type);
enum_conv(GroupSemantics, group_semantics);
} // namespace serde

#include "serde/macro-pop.hpp"

#include "../macros/unwrap.hpp"

namespace jingle {
auto parse(const xml::Node& node) -> std::optional<Jingle> {
    unwrap_mut(parsed, (serde::load<serde::XmlFormat, jingle::Jingle>(node)));
    return std::move(parsed);
}

auto deparse(const Jingle& jingle) -> std::optional<xml::Node> {
    unwrap_mut(node, jingle.dump<serde::XmlFormat>());
    node.name = "jingle";
    return node;
}
} // namespace jingle

// #include "macro.hpp"

// auto main() -> int {
//     // auto f = serde::XmlFormat();
//     // auto d = std::optional<int>();
//     // auto p = xml::Node();
//     // serde::deserialize(f, "", p, d);
//     unwrap(text, read_file("data.xml"));
//     unwrap(node, xml::parse(from_span(text)));
//     unwrap(jingle, node.find_first_child("jingle"));
//     unwrap(parsed, (serde::load<serde::XmlFormat, jingle::Jingle>(jingle)));
//     std::println("action={}", std::to_underlying(parsed.action));
//     auto dump = parsed.dump<serde::XmlFormat>();
//     dump.name = "jingle";
//     write_file("new.xml", to_span(xml::deparse(dump)));
//     return 0;
// }
