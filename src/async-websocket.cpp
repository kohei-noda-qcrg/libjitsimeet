#include <coop/thread.hpp>

#include "async-websocket.hpp"

namespace ws::client {
auto AsyncContext::init(coop::TaskInjector& injector, const ContextParams& params) -> bool {
    Context::handler = [this, &injector](const std::span<const std::byte> data) {
        injector.inject_task(handler(data));
    };
    return Context::init(params);
}

auto AsyncContext::process_until_finish() -> coop::Async<void> {
    while(state == ws::client::State::Connected) {
        co_await coop::run_blocking([this]() { process(); });
    }
}
} // namespace ws::client
