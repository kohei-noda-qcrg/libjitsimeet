#pragma once
#include "../xml/xml.hpp"

namespace xmpp::ns {
constexpr auto disco_info  = "http://jabber.org/protocol/disco#info";
constexpr auto caps        = "http://jabber.org/protocol/caps";
constexpr auto nick        = "http://jabber.org/protocol/nick";
constexpr auto muc         = "http://jabber.org/protocol/muc";
constexpr auto muc_user    = "http://jabber.org/protocol/muc#user";
constexpr auto jitsi_focus = "http://jitsi.org/protocol/focus";
} // namespace xmpp::ns

namespace xmpp::elm {
// templates
inline const auto iq = xml::Node{
    .name  = "iq",
    .attrs = {
        {"xmlns", "jabber:client"},
        // from
        // to
        // id
        // type
    },
};
inline const auto open = xml::Node{
    .name  = "open",
    .attrs = {
        {"version", "1.0"},
        {"xmlns", "urn:ietf:params:xml:ns:xmpp-framing"},
        // to
    },
};
inline const auto conference = xml::Node{
    .name  = "conference",
    .attrs = {
        {"xmlns", ns::jitsi_focus},
        // machine-uid
        // room
    },
};
inline const auto property = xml::Node{
    .name  = "property",
    .attrs = {
        // name
        // value
    },
};
inline const auto auth = xml::Node{
    .name  = "auth",
    .attrs = {
        {"xmlns", "urn:ietf:params:xml:ns:xmpp-sasl"},
        {"mechanism", "ANONYMOUS"},
    },
};
inline const auto bind = xml::Node{
    .name  = "bind",
    .attrs = {
        {"xmlns", "urn:ietf:params:xml:ns:xmpp-bind"},
    },
};
inline const auto query = xml::Node{
    .name  = "query",
    .attrs = {
        {"xmlns", ns::disco_info},
    },
};
inline const auto services = xml::Node{
    .name  = "services",
    .attrs = {
        {"xmlns", "urn:xmpp:extdisco:2"},
    },
};
inline const auto presence = xml::Node{
    .name  = "presence",
    .attrs = {
        {"xmlns", "jabber:client"},
        // to
    },
};
inline const auto identity = xml::Node{
    .name  = "identity",
    .attrs = {
        // catgegory
        // name
        // type
        // xml:lang
    },
};
inline const auto feature = xml::Node{
    .name  = "feature",
    .attrs = {
        // var
    },
};
inline const auto caps = xml::Node{
    .name  = "c",
    .attrs = {
        {"xmlns", ns::caps},
        // hash
        // node
        // ver
    },
};
inline const auto nick = xml::Node{
    .name  = "nick",
    .attrs = {
        {"xmlns", ns::nick},
    },
};
inline const auto muc = xml::Node{
    .name  = "x",
    .attrs = {
        {"xmlns", ns::muc},
    },
};
inline const auto ecaps2 = xml::Node{
    .name  = "c",
    .attrs = {
        {"xmlns", "urn:xmpp:caps"},
    },
};
inline const auto hash = xml::Node{
    .name  = "hash",
    .attrs = {
        {"xmlns", "urn:xmpp:hashes:2"},
        // algo
    },
};
inline const auto ping = xml::Node{
    .name  = "ping",
    .attrs = {
        {"xmlns", "urn:xmpp:ping"},
    },
};
} // namespace xmpp::elm
