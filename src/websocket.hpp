#pragma once
#include <functional>
#include <span>
#include <string_view>

namespace ws {
struct Connection;

enum class RxResult {
    Ignored,
    Handled,
    Complete,
};

using Rx = std::function<RxResult(const std::span<std::byte>)>;

auto connect(const char* address, const char* path) -> Connection*;
auto free_connection(Connection* conn) -> void;
auto send_str(Connection* conn, std::string_view str) -> void;
auto add_rx(Connection* conn, Rx rx) -> void;
} // namespace ws
