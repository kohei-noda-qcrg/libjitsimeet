#pragma once
#include <array>
#include <string_view>

template <class T, size_t N>
auto make_array(T (&&values)[N]) -> std::array<T, N> {
    return std::to_array(values);
}

template <class T, size_t N>
struct StrTable {
    using Pair = std::pair<T, const char*>;

    std::array<Pair, N> array;

    auto find(const T value) const -> const Pair* {
        for(const auto& e : array) {
            if(e.first == value) {
                return &e;
            }
        }
        return nullptr;
    }

    auto find(const std::string_view str) const -> const Pair* {
        for(const auto& e : array) {
            if(e.second == str) {
                return &e;
            }
        }
        return nullptr;
    }
};

template <class T, size_t N>
auto make_str_table(std::pair<T, const char*> (&&values)[N]) -> StrTable<T, N> {
    return StrTable<T, N>{
        .array = std::to_array(values),
    };
}
