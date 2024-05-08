#include "jid.hpp"

namespace xmpp {
auto Jid::as_bare() const -> std::string {
    return node + "@" + domain;
}

auto Jid::as_full() const -> std::string {
    return node + "@" + domain + "/" + resource;
}

auto Jid::operator==(const Jid& o) const -> bool {
    return node == o.node && domain == o.domain && resource == o.resource;
}

auto Jid::parse(std::string_view str) -> std::optional<Jid> {
    const auto at = str.find('@');
    if(at == str.npos) {
        return std::nullopt;
    }
    const auto node = str.substr(0, at);
    const auto sl   = str.find('/');
    if(sl == str.npos) {
        const auto domain = str.substr(at + 1);
        return Jid{std::string(node), std::string(domain), ""};
    }
    const auto domain   = str.substr(at + 1, sl - at - 1);
    const auto resource = str.substr(sl + 1);
    return Jid{std::string(node), std::string(domain), std::string(resource)};
}
} // namespace xmpp
