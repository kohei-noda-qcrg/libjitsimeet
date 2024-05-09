#include "negotiator.hpp"
#include "../assert.hpp"
#include "../config.hpp"
#include "../util/coroutine.hpp"
#include "../xml/xml.hpp"
#include "elements.hpp"

namespace xmpp {
namespace {
auto negotiate(Negotiator* const negotiator) -> Negotiator::Worker::Generator {
    if(config::debug_xmpp_connection) {
        PRINT("negotiation started");
    }
    auto& self = *negotiator;
    // open
    {
        const auto open = xmpp::elm::open.clone().append_attrs({
            {"to", self.host},
        });
        self.callbacks->send_payload(xml::deparse(open));
        co_yield true;

        while(true) {
            const auto response = xml::parse(self.worker_arg).as_value();
            if(response.name == "open" && response.is_attr_equal("from", self.host)) {
                break;
            }
            co_yield false;
        }
    }
    // wait for stream
    {
        co_yield true;

        while(true) {
            const auto response = xml::parse(self.worker_arg).as_value();
            if(response.name == "stream:features") {
                break;
            }
            co_yield false;
        }
    }
    // auth
    {
        const auto auth = xmpp::elm::auth;
        self.callbacks->send_payload(xml::deparse(auth));
        co_yield true;

        const auto response = xml::parse(self.worker_arg).as_value();
        DYN_ASSERT(response.name == "success");
    }
    // open
    {
        const auto open = xmpp::elm::open.clone().append_attrs({
            {"to", self.host},
        });
        self.callbacks->send_payload(xml::deparse(open));
        co_yield true;

        const auto response = xml::parse(self.worker_arg).as_value();
        DYN_ASSERT(response.name == "open");
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
        co_yield true;

        while(true) {
            const auto response = xml::parse(self.worker_arg).as_value();
            if(response.name != "iq") {
                co_yield false;
                continue;
            }
            DYN_ASSERT(response.is_attr_equal("id", id), "unexpected iq");
            const auto jid_n = response.find_first_child("bind", "jid");
            DYN_ASSERT(jid_n != nullptr, "jid not found in iq response");
            auto jid_o = Jid::parse(jid_n->data);
            DYN_ASSERT(jid_o.has_value());
            self.jid = *std::move(jid_o);
            break;
        }
        if(config::debug_xmpp_connection) {
            print("jid: ", self.jid.as_full());
        }
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
        co_yield true;

        while(true) {
            const auto response = xml::parse(self.worker_arg).as_value();
            if(response.name != "iq") {
                co_yield false;
                continue;
            }
            DYN_ASSERT(response.is_attr_equal("id", id), "unexpected iq");
            DYN_ASSERT(response.is_attr_equal("type", "result"), "unexpected iq");
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
        co_yield true;

        while(true) {
            const auto response = xml::parse(self.worker_arg).as_value();
            if(response.name != "iq") {
                co_yield false;
                continue;
            }
            DYN_ASSERT(response.is_attr_equal("id", id), "unexpected iq");
            DYN_ASSERT(response.is_attr_equal("type", "result"), "unexpected iq");
            const auto services = response.find_first_child("services");
            DYN_ASSERT(services != nullptr);
            if(auto sv_o = parse_services(*services); sv_o) {
                self.external_services = std::move(*sv_o);
            }
            break;
        }
    }
    co_return true;
}
} // namespace

auto Negotiator::generate_iq_id() -> std::string {
    return build_string("iq_", (iq_serial += 1));
}

auto Negotiator::start_negotiation() -> void {
    worker.start(negotiate, this);
    worker.resume();
}

auto Negotiator::feed_payload(std::string_view payload) -> bool {
    worker_arg = payload;
    worker.resume();
    return worker.done();
}

auto Negotiator::create(std::string host, NegotiatorCallbacks* const callbacks) -> std::unique_ptr<Negotiator> {
    return std::unique_ptr<Negotiator>(new Negotiator{
        .host      = std::move(host),
        .callbacks = callbacks,
    });
}
} // namespace xmpp
