#include "negotiator.hpp"
#include "../macros/logger.hpp"
#include "../util/coroutine.hpp"
#include "../xml/xml.hpp"
#include "elements.hpp"

#define CUTIL_MACROS_PRINT_FUNC(...) LOG_ERROR(logger, __VA_ARGS__)
#include "../macros/unwrap.hpp"

namespace xmpp {
namespace {
auto logger = Logger("xmpp");

auto negotiate(Negotiator* const negotiator) -> Negotiator::Worker::Generator {
    constexpr auto error_value = FeedResult::Error;

    LOG_DEBUG(logger, "negotiation started");
    auto& self = *negotiator;
    // open
    {
        const auto open = xmpp::elm::open.clone().append_attrs({
            {"to", self.host},
        });
        self.callbacks->send_payload(xml::deparse(open));
        co_yield FeedResult::Continue;

        while(true) {
            const auto response = xml::parse(self.worker_arg).value();
            if(response.name == "open" && response.is_attr_equal("from", self.host)) {
                break;
            }
            co_yield FeedResult::Continue;
        }
    }
    // wait for stream
    {
        co_yield FeedResult::Continue;

        while(true) {
            const auto response = xml::parse(self.worker_arg).value();
            if(response.name == "stream:features") {
                break;
            }
            co_yield FeedResult::Continue;
        }
    }
    // auth
    {
        const auto auth = xmpp::elm::auth;
        self.callbacks->send_payload(xml::deparse(auth));
        co_yield FeedResult::Continue;

        const auto response = xml::parse(self.worker_arg).value();
        co_ensure_v(response.name == "success");
    }
    // open
    {
        const auto open = xmpp::elm::open.clone().append_attrs({
            {"to", self.host},
        });
        self.callbacks->send_payload(xml::deparse(open));
        co_yield FeedResult::Continue;

        const auto response = xml::parse(self.worker_arg).value();
        co_ensure_v(response.name == "open");
    }
    // bind
    {
        const auto id = self.generate_iq_id();
        const auto iq = xmpp::elm::iq.clone()
                            .append_attrs({
                                xml::Attribute{"id", id},
                                xml::Attribute{"type", "set"},
                            })
                            .append_children({
                                xmpp::elm::bind,
                            });
        self.callbacks->send_payload(xml::deparse(iq));
        co_yield FeedResult::Continue;

        while(true) {
            const auto response = xml::parse(self.worker_arg).value();
            if(response.name != "iq") {
                co_yield FeedResult::Continue;
                continue;
            }
            co_ensure_v(response.is_attr_equal("id", id), "unexpected iq");
            co_unwrap_v(jid_n, response.find_first_child("bind", "jid"), "jid not found in iq response");
            co_unwrap_v_mut(jid, Jid::parse(jid_n.data));
            self.jid = std::move(jid);
            break;
        }
        LOG_DEBUG(logger, "jid: {}", self.jid.as_full());
    }
    // disco
    {
        const auto id        = self.generate_iq_id();
        const auto focus_jid = Jid{"focus", std::string("auth.") + self.host, "focus"};
        const auto iq        = xmpp::elm::iq.clone()
                            .append_attrs({
                                {"id", id},
                                {"type", "get"},
                                {"from", self.jid.as_full()},
                                {"to", self.host},
                            })
                            .append_children({
                                xmpp::elm::query,
                            });
        self.callbacks->send_payload(xml::deparse(iq));
        co_yield FeedResult::Continue;

        while(true) {
            const auto response = xml::parse(self.worker_arg).value();
            if(response.name != "iq") {
                co_yield FeedResult::Continue;
                continue;
            }
            co_ensure_v(response.is_attr_equal("id", id), "unexpected iq");
            co_ensure_v(response.is_attr_equal("type", "result"), "unexpected iq");
            // TODO: parse disco
            break;
        }
    }
    // disco ext
    {
        const auto id = self.generate_iq_id();
        const auto iq = xmpp::elm::iq.clone()
                            .append_attrs({
                                {"id", id},
                                {"type", "get"},
                                {"from", self.jid.as_full()},
                                {"to", self.host},
                            })
                            .append_children({
                                xmpp::elm::services,
                            });
        self.callbacks->send_payload(xml::deparse(iq));
        co_yield FeedResult::Continue;

        while(true) {
            const auto response = xml::parse(self.worker_arg).value();
            if(response.name != "iq") {
                co_yield FeedResult::Continue;
                continue;
            }
            co_ensure_v(response.is_attr_equal("id", id), "unexpected iq");
            co_ensure_v(response.is_attr_equal("type", "result"), "unexpected iq");
            co_unwrap_v(services, response.find_first_child("services"));
            co_ensure_v(services.is_attr_equal("xmlns", xmpp::ns::xmpp_extdisco));
            if(auto sv_o = parse_services(services); sv_o) {
                self.external_services = std::move(*sv_o);
            }
            break;
        }
    }
    co_return FeedResult::Done;
}
} // namespace

auto Negotiator::generate_iq_id() -> std::string {
    return std::format("iq_{}", (iq_serial += 1));
}

auto Negotiator::start_negotiation() -> void {
    worker.start(negotiate, this);
    worker.resume();
}

auto Negotiator::feed_payload(std::string_view payload) -> FeedResult {
    worker_arg = payload;
    return worker.resume();
}

auto Negotiator::create(std::string host, NegotiatorCallbacks* const callbacks) -> std::unique_ptr<Negotiator> {
    return std::unique_ptr<Negotiator>(new Negotiator{
        .host      = std::move(host),
        .callbacks = callbacks,
    });
}
} // namespace xmpp
