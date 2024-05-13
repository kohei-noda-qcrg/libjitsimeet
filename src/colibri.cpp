#include "config.hpp"
#include "jingle/jingle.hpp"
#include "unwrap.hpp"
#include "uri.hpp"
#include "websocket.hpp"

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

auto test() -> bool {
    unwrap_ob(ws_uri, URI::parse("wss://jitsi-master-vs.saver.jp:443/colibri-ws/jvb1/7f0696759647c655/zpspubhvu0nwutih?pwd=2tu4ogaldmo38co1uo6l7lbuga"));
    print("domain:", ws_uri.domain);
    print("path:", ws_uri.path);
    print("port:", ws_uri.port);
    return true;
}

auto setup_colibri(const jingle::Jingle& initiate_jingle, const bool secure) -> bool {
    unwrap_pb(transport, find_transport(initiate_jingle));
    unwrap_ob(ws_uri, URI::parse(transport.websocket));
    if(config::debug_colibri) {
        PRINT("connecting to colibri at ", transport.websocket);
    }
    unwrap_pb_mut(conn, ws::create_connection(std::string(ws_uri.domain).data(), ws_uri.port, std::string(ws_uri.path).data(), secure));
    ws::send_str(&conn, R"({"colibriClass":"ReceiverVideoConstraints","lastN":1})");
    return true;
}
} // namespace colibri
