#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace xml {
struct Node;
}

namespace xmpp {
struct Service {
    std::string type;
    std::string host;
    std::string name       = "";
    std::string transport  = "";
    std::string username   = "";
    std::string password   = "";
    std::string expires    = "";
    uint16_t    port       = 0;
    bool        restricted = false;
};

auto parse_services(const xml::Node& services) -> std::optional<std::vector<Service>>;
} // namespace xmpp
