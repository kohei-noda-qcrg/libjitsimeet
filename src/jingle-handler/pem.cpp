#include <format>
#include <string>

#include "../crypto/base64.hpp"

namespace pem {
auto encode(const std::string_view label, const std::span<const std::byte> bytes) -> std::string {
    auto r   = std::format("-----BEGIN {}-----\n", label);
    auto b64 = crypto::base64::encode(bytes);
    auto pos = 0uz;
    while(pos + 64 < b64.size()) {
        r += std::format("{}\n", b64.substr(pos, 64));
        pos += 64;
    }
    r += std::format("{}\n", b64.substr(pos));
    r += std::format("-----END {}-----", label);
    return r;
}
} // namespace pem
