#include <array>
#include <span>

#include <openssl/evp.h>

namespace sha {
namespace {
template <size_t N>
auto calc_generic(const std::span<const std::byte> str, const char* const algo) -> std::array<std::byte, N> {
    auto buf = std::array<std::byte, N>();
    auto md  = EVP_get_digestbyname(algo);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, NULL);
    EVP_DigestUpdate(ctx, str.data(), str.size());
    EVP_DigestFinal_ex(ctx, reinterpret_cast<unsigned char*>(buf.data()), NULL);
    EVP_MD_CTX_free(ctx);
    return buf;
}
} // namespace

auto calc_sha1(const std::span<const std::byte> str) -> std::array<std::byte, 20> {
    return calc_generic<20>(str, "SHA1");
}

auto calc_sha256(const std::span<const std::byte> str) -> std::array<std::byte, 32> {
    return calc_generic<32>(str, "SHA256");
}
} // namespace sha
