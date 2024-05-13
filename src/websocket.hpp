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

auto create_connection(const char* address, uint32_t port, const char* path, bool secure) -> Connection*;
auto free_connection(Connection* conn) -> void;
auto send_str(Connection* conn, std::string_view str) -> void;
auto add_receiver(Connection* conn, Receiver receiver) -> void;
} // namespace ws
