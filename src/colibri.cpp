#include "colibri.hpp"
#include "config.hpp"
#include "unwrap.hpp"
#include "uri.hpp"

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
    ws::send_str(ws_conn, payload);
}

Colibri::~Colibri() {
    ws::free_connection(ws_conn);
}

auto Colibri::connect(const jingle::Jingle& initiate_jingle, const bool secure) -> std::optional<Colibri> {
    unwrap_po(transport, find_transport(initiate_jingle));
    unwrap_oo(ws_uri, URI::parse(transport.websocket));
    if(config::debug_colibri) {
        PRINT("connecting to colibri at ", transport.websocket);
    }
    unwrap_po_mut(conn, ws::create_connection(std::string(ws_uri.domain).data(), ws_uri.port, std::string(ws_uri.path).data(), secure));
    return Colibri{.ws_conn = &conn};
}
} // namespace colibri
