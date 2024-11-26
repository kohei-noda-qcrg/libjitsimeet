#include "async-websocket.hpp"

namespace ws::client {
auto AsyncContext::init(coop::TaskInjector& injector, const ContextParams& params) -> bool {
    Context::handler = [this, &injector](const std::span<const std::byte> data) {
        injector.inject_task(handler(data));
    };
    return Context::init(params);
}
} // namespace ws::client
