#pragma once
#include "jingle/jingle.hpp"
#include "websocket.hpp"

namespace colibri {
struct Colibri {
    ws::Connection* ws_conn;

    auto set_last_n(int n) -> void;

    ~Colibri();

    static auto connect(const jingle::Jingle& initiate_jingle, bool secure) -> std::optional<Colibri>;
};
} // namespace colibri
