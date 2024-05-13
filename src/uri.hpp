#pragma once
#include <optional>
#include <string_view>

struct URI {
    std::string_view protocol;
    std::string_view domain;
    std::string_view path;
    uint32_t         port;

    static auto parse(std::string_view str) -> std::optional<URI>;
};
