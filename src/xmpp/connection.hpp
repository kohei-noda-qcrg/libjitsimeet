#pragma once
#include <functional>
#include <string_view>

#include "extdisco.hpp"
#include "jid.hpp"

namespace xmpp {
struct Connection;

struct NegotiaionResult {
    Jid                  jid;
    std::vector<Service> external_services;
};

auto create(std::string host, std::function<void(std::string_view)> tx) -> Connection*;
auto finish(Connection* conn) -> NegotiaionResult;
auto start_negotiation(Connection* conn) -> void;
auto resume_negotiation(Connection* conn, const std::string_view payload) -> bool;
} // namespace xmpp
