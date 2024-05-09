#pragma once
#include "util/assert.hpp"

#define PANIC(...) line_panic({__FILE__, __LINE__} __VA_OPT__(, ) __VA_ARGS__);
#define PRINT(...) line_print({__FILE__, __LINE__} __VA_OPT__(, ) __VA_ARGS__);
#define WARN(...) line_warn({__FILE__, __LINE__} __VA_OPT__(, ) __VA_ARGS__);
#define DYN_ASSERT(cond, ...) line_assert((cond), {__FILE__, __LINE__} __VA_OPT__(, ) __VA_ARGS__);

#define assert_v(cond, ret, ...) \
    if(!(cond)) {                \
        WARN(__VA_ARGS__);       \
        return ret;              \
    }

#define assert_n(cond, ...) assert_v(cond, , __VA_ARGS__)
#define assert_o(cond, ...) assert_v(cond, std::nullopt, __VA_ARGS__)
#define assert_b(cond, ...) assert_v(cond, false, __VA_ARGS__)
#define assert_p(cond, ...) assert_v(cond, nullptr, __VA_ARGS__)

