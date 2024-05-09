#pragma once
#include <functional>
#include <span>
#include <string_view>

namespace ws {
struct Connection;

enum class ReceiverResult {
    Ignored,
    Handled,
    Complete,
};

using Receiver = std::function<ReceiverResult(const std::span<std::byte>)>;

auto connect(const char* address, const char* path) -> Connection*;
auto free_connection(Connection* conn) -> void;
auto send_str(Connection* conn, std::string_view str) -> void;
auto add_receiver(Connection* conn, Receiver receiver) -> void;
} // namespace ws
