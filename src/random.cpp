#include <random>

namespace rng {
namespace {
auto engine = std::mt19937((std::random_device())());
} // namespace

auto generate_random_uint32() -> uint32_t {
    return engine();
}
} // namespace rng
