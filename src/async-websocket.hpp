#pragma once
#include <coop/task-injector.hpp>

#include "ws/client.hpp"

namespace ws::client {
using OnDataReceivedAsync = coop::Async<void>(std::span<const std::byte> payload);

struct AsyncContext : Context {
    std::function<OnDataReceivedAsync> handler;

    auto init(coop::TaskInjector& injector, const ContextParams& params) -> bool;
};
} // namespace ws::client
