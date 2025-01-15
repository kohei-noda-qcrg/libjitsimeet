#include "extdisco.hpp"
#include "../macros/logger.hpp"
#include "../util/charconv.hpp"
#include "../xml/xml.hpp"

#define CUTIL_MACROS_PRINT_FUNC(...) LOG_ERROR(logger, __VA_ARGS__)
#include "../macros/unwrap.hpp"

namespace xmpp {
namespace {
auto logger = Logger("xmpp");

auto parse_service(const xml::Node& node) -> std::optional<Service> {
    auto r          = Service{};
    auto found_type = false;
    auto found_host = false;

    for(const auto& a : node.attrs) {
        if(a.key == "type") {
            r.type     = a.value;
            found_type = true;
        } else if(a.key == "host") {
            r.host     = a.value;
            found_host = true;
        } else if(a.key == "name") {
            r.name = a.value;
        } else if(a.key == "transport") {
            r.transport = a.value;
        } else if(a.key == "username") {
            r.username = a.value;
        } else if(a.key == "password") {
            r.password = a.value;
        } else if(a.key == "port") {
            unwrap(num, from_chars<uint16_t>(a.value));
            r.port = num;
        } else if(a.key == "restricted") {
            // TODO: should be a.value.tolower() == "parse"
            if(a.value == "1" || a.value == "parse") {
                r.restricted = true;
            } else if(a.value == "0") {
                r.restricted = false;
            } else {
                bail("unknown restricted");
            }
        } else {
            LOG_WARN(logger, "unhandled attribute ", a.key);
        }
    }
    if(!found_type || !found_host) {
        bail("required attributes not found");
    }
    for(const auto& c : node.children) {
        if(0) {
        } else {
            LOG_WARN(logger, "unhandled child ", c.name);
        }
    }
    return r;
}
} // namespace
auto parse_services(const xml::Node& services) -> std::optional<std::vector<Service>> {
    auto r = std::vector<Service>();
    for(const auto& service : services.children) {
        if(service.name != "service") {
            continue;
        }
        if(const auto s = parse_service(service); s) {
            r.push_back(*s);
        } else {
            LOG_ERROR(logger, "failed to parse service");
            continue;
        }
    }
    return r;
}
} // namespace xmpp
