#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include "../util/coroutine.hpp"
#include "extdisco.hpp"
#include "jid.hpp"

namespace xmpp {
struct NegotiatorCallbacks {
    virtual auto send_payload(std::string_view /*payload*/) -> void = 0;

    virtual ~NegotiatorCallbacks(){};
};

struct Negotiator {
    using Worker = CoRoutine<bool>;

    // constant
    std::string          host;
    NegotiatorCallbacks* callbacks;

    // worker
    std::string_view worker_arg;
    Worker           worker;

    // state
    Jid                  jid;
    std::vector<Service> external_services;
    static inline int    iq_serial;

    auto generate_iq_id() -> std::string;
    auto start_negotiation() -> void;
    auto feed_payload(std::string_view payload) -> bool;

    static auto create(std::string host, NegotiatorCallbacks* callbacks) -> std::unique_ptr<Negotiator>;
};
} // namespace xmpp
