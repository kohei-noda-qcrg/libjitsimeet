#include "uri.hpp"
#include "macros/unwrap.hpp"
#include "util/assert.hpp"
#include "util/charconv.hpp"

namespace {
auto cut_head(std::string_view& source, const std::string_view dlm) -> std::optional<std::string_view> {
    const auto i = source.find(dlm);
    assert_o(i != source.npos);
    const auto ret = source.substr(0, i);
    source         = source.substr(i + dlm.size());
    return ret;
}
} // namespace

auto URI::parse(std::string_view str) -> std::optional<URI> {
    unwrap_oo(protocol, cut_head(str, "://"));
    unwrap_oo(domain, cut_head(str, ":"));
    unwrap_oo(port_str, cut_head(str, "/"));
    unwrap_oo(port, from_chars<uint32_t>(port_str));
    const auto path = str;
    return URI{
        .protocol = protocol,
        .domain   = domain,
        .path     = path,
        .port     = port,
    };
}
