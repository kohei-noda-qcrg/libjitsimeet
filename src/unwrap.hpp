#pragma once
#include "assert.hpp"

#define unwrap_re(var, res)        \
    const auto var##_r = res;      \
    if(!var##_r) {                 \
        return var##_r.as_error(); \
    }                              \
    const auto& var = var##_r.as_value();

#define unwrap_ov(var, opt, const, ret, ...) \
    const auto var##_o = opt;                \
    if(!var##_o.has_value()) {               \
        WARN(__VA_ARGS__);                   \
        return ret;                          \
    }                                        \
    const auto& var = *var##_o;

#define unwrap_ob(var, opt, ...) unwrap_ov(var, opt, const, false, __VA_ARGS__)
#define unwrap_ob_mut(var, opt, ...) unwrap_ov(var, opt, , false, __VA_ARGS__)
#define unwrap_oo(var, opt, ...) unwrap_ov(var, opt, const, std::nullopt, __VA_ARGS__)
#define unwrap_oo_mut(var, opt, ...) unwrap_ov(var, opt, , std::nullopt, __VA_ARGS__)
#define unwrap_on(var, opt, ...) unwrap_ov(var, opt, const, , __VA_ARGS__)
#define unwrap_on_mut(var, opt, ...) unwrap_ov(var, opt, , , __VA_ARGS__)

#define unwrap_pv(var, opt, const, ret, ...) \
    const auto var##_p = opt;                \
    if(var##_p == nullptr) {                 \
        WARN(__VA_ARGS__);                   \
        return ret;                          \
    }                                        \
    const auto& var = *var##_p;

#define unwrap_pb(var, opt, ...) unwrap_pv(var, opt, const, false, __VA_ARGS__)
#define unwrap_pb_mut(var, opt, ...) unwrap_pv(var, opt, , false, __VA_ARGS__)
#define unwrap_po(var, opt, ...) unwrap_pv(var, opt, const, std::nullopt, __VA_ARGS__)
#define unwrap_pn(var, opt, ...) unwrap_pv(var, opt, const, , __VA_ARGS__)
#define unwrap_pn_mut(var, opt, ...) unwrap_pv(var, opt, , , __VA_ARGS__)
