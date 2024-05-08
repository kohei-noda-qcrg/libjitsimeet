#pragma once
#include <optional>
#include <string>

namespace xmpp {
struct Jid {
    std::string node;
    std::string domain;
    std::string resource;

    auto as_bare() const -> std::string;
    auto as_full() const -> std::string;

    auto operator==(const Jid& o) const -> bool;

    static auto parse(std::string_view str) -> std::optional<Jid>;
};
} // namespace xmpp
