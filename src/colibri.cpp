#include "colibri.hpp"
#include "config.hpp"
#include "macros/unwrap.hpp"
#include "uri.hpp"
#include "util/assert.hpp"

namespace colibri {
namespace {
auto find_transport(const jingle::Jingle& jingle) -> const jingle::Jingle::Content::IceUdpTransport* {
    for(const auto& c : jingle.contents) {
        if(!c.transports.empty()) {
            return &c.transports[0];
        }
    }
    return nullptr;
}
} // namespace

auto Colibri::set_last_n(const int n) -> void {
    const auto payload = build_string(R"({"colibriClass":"ReceiverVideoConstraints","lastN":)", n, "}");
    ensure(ws_context.send(payload));
}

Colibri::~Colibri() {
}

auto Colibri::connect(const jingle::Jingle& initiate_jingle, const bool secure) -> std::unique_ptr<Colibri> {
    unwrap(transport, find_transport(initiate_jingle));
    unwrap(ws_uri, URI::parse(transport.websocket));
    if(config::debug_colibri) {
        line_print("connecting to colibri at ", transport.websocket);
    }

    const auto uri_domain = std::string(ws_uri.domain);
    const auto uri_path   = std::string(ws_uri.path);
    auto       obj        = std::unique_ptr<Colibri>(new Colibri());
    ensure(obj->ws_context.init({
        .address   = uri_domain.data(),
        .path      = uri_path.data(),
        .protocol  = "xmpp",
        .port      = ws_uri.port,
        .ssl_level = secure ? ws::client::SSLLevel::Enable : ws::client::SSLLevel::TrustSelfSigned,
    }));
    return obj;
}
} // namespace colibri
