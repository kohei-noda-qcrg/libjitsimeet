#pragma once
#include <array>
#include <span>

namespace sha {
auto calc_sha1(std::span<const std::byte> data) -> std::array<std::byte, 20>;
auto calc_sha256(std::span<const std::byte> str) -> std::array<std::byte, 32>;
} // namespace sha
