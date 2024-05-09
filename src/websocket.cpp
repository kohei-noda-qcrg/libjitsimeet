#include <thread>
#include <vector>

#include <libwebsockets.h>

#include "assert.hpp"
#include "config.hpp"
#include "websocket.hpp"

namespace ws {
enum class ConnectionState {
    Initialized,
    Connected,
    Destroyed,
};

struct Connection {
    lws_context*          context;
    lws*                  wsi;
    std::vector<Receiver> receivers;
    ConnectionState       conn_state;
    std::thread           worker;
};

namespace {
auto write_back(lws* const wsi, const void* const data, size_t size) -> int {
    auto buffer       = std::vector<std::byte>(LWS_SEND_BUFFER_PRE_PADDING + size + LWS_SEND_BUFFER_POST_PADDING);
    auto payload_head = buffer.data() + LWS_SEND_BUFFER_PRE_PADDING;
    memcpy(payload_head, data, size);
    return lws_write(wsi, std::bit_cast<unsigned char*>(payload_head), size, LWS_WRITE_TEXT);
}

auto write_back_str(lws* const wsi, const std::string_view str) -> int {
    return write_back(wsi, str.data(), str.size());
}

auto xmpp_callback(lws* wsi, lws_callback_reasons reason, void* const /*user*/, void* const in, const size_t len) -> int {
    const auto proto = lws_get_protocol(wsi);
    if(config::debug_websocket) {
        PRINT(__func__, " reason=", int(reason), " protocol=", proto);
    }
    if(proto == NULL) {
        return 0;
    }
    if(len >= proto->rx_buffer_size) {
        WARN("rx packet size limit exceeded");
    }

    const auto conn = std::bit_cast<Connection*>(proto->user);
    switch(reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        if(config::debug_websocket) {
            PRINT(__func__, " connection established");
        }
        conn->conn_state = ConnectionState::Connected;
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        if(config::debug_websocket) {
            PRINT(__func__, " connection error");
        }
        conn->conn_state = ConnectionState::Destroyed;
        break;
    case LWS_CALLBACK_CLOSED:
        if(config::debug_websocket) {
            PRINT(__func__, " connection close");
        }
        conn->conn_state = ConnectionState::Destroyed;
        break;
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        if(config::dump_websocket_packets) {
            auto str = std::string_view(std::bit_cast<char*>(in));
            PRINT(">>> ", str);
        }
        auto&      rxs  = conn->receivers;
        const auto data = std::span<std::byte>(std::bit_cast<std::byte*>(in), len);
        for(auto i = rxs.begin(); i < rxs.end(); i += 1) {
            switch((*i)(data)) {
            case ReceiverResult::Ignored:
                break;
            case ReceiverResult::Complete:
                i = rxs.erase(i);
                [[fallthrough]];
            case ReceiverResult::Handled:
                i = rxs.end();
                break;
            }
        }
    } break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        if(config::debug_websocket) {
            PRINT(__func__, " writeable");
        }
        break;
    default:
        if(config::debug_websocket) {
            PRINT(__func__, " unhandled callback");
        }
        break;
    }

    return 0;
}

auto connection_worker_main(Connection* const conn) -> void {
    while(conn->conn_state != ConnectionState::Destroyed) {
        lws_service(conn->context, 50);
    }
    lws_context_destroy(conn->context);
}

} // namespace

auto create_connection(const char* const address, const char* const path, const bool secure) -> Connection* {
    auto conn = std::make_unique<Connection>(Connection({
        .context    = NULL,
        .wsi        = NULL,
        .conn_state = ConnectionState::Initialized,
        .worker     = {},
    }));

    const auto xmpp = lws_protocols{
        .name           = "xmpp",
        .callback       = xmpp_callback,
        .rx_buffer_size = 0x1000 * 1,
        .user           = conn.get(),
        .tx_packet_size = 0x1000 * 1,
    };
    const auto protocols = std::array{&xmpp, (const lws_protocols*)NULL};

    const auto context_creatin_info = lws_context_creation_info{
        .port       = CONTEXT_PORT_NO_LISTEN,
        .gid        = gid_t(-1),
        .uid        = uid_t(-1),
        .options    = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
        .pprotocols = (const lws_protocols**)protocols.data(),
    };

    const auto context = lws_create_context(&context_creatin_info);
    DYN_ASSERT(context != NULL);

    const auto port      = 443;
    const auto host      = build_string(address, ":", port);
    const auto ssl_flags = secure ? LCCSCF_USE_SSL : LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

    const auto client_connect_info = lws_client_connect_info{
        .context                   = context,
        .address                   = address,
        .port                      = port,
        .ssl_connection            = ssl_flags,
        .path                      = path,
        .host                      = host.data(),
        .protocol                  = xmpp.name,
        .ietf_version_or_minus_one = -1,
    };
    const auto wsi = lws_client_connect_via_info(&client_connect_info);
    DYN_ASSERT(wsi != NULL);
    lws_callback_on_writable(wsi);

    while(conn->conn_state == ConnectionState::Initialized) {
        lws_service(context, 50);
    }

    conn->context = context;
    conn->wsi     = wsi;
    conn->worker  = std::thread(connection_worker_main, conn.get());

    return conn.release();
}

auto free_connection(Connection* const conn) -> void {
    conn->worker.join();
    delete conn;
}

auto send_str(Connection* const conn, const std::string_view str) -> void {
    if(config::dump_websocket_packets) {
        PRINT("<<< ", str);
    }
    write_back_str(conn->wsi, str);
}

auto add_receiver(Connection* conn, const Receiver receiver) -> void {
    conn->receivers.push_back(receiver);
}
} // namespace ws
