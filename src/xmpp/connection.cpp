#include "connection.hpp"
#include "../assert.hpp"
#include "../config.hpp"
#include "../util/coroutine.hpp"
#include "../xml/xml.hpp"
#include "elements.hpp"

namespace xmpp {
using Worker = CoRoutine<bool>;

struct Connection {
    std::string                           host;
    Jid                                   jid;
    std::vector<Service>                  external_services;
    Worker                                worker;
    std::string_view                      rx_payload;
    std::function<void(std::string_view)> tx;
};

namespace {
auto negotiate(Connection* const conn) -> Worker::Generator {
    if(config::debug_xmpp_connection) {
        PRINT("negotiation started");
    }
    auto iq_serial = 0;
    // open
    {
        const auto open = xmpp::elm::open.clone().append_attrs({
            {"to", conn->host},
        });
        conn->tx(xml::deparse(open));
        co_yield true;

        while(true) {
            const auto response = xml::parse(conn->rx_payload).as_value();
            if(response.name == "open" && response.is_attr_equal("from", conn->host)) {
                break;
            }
            co_yield false;
        }
    }
    // wait for stream
    {
        co_yield true;

        while(true) {
            const auto response = xml::parse(conn->rx_payload).as_value();
            if(response.name == "stream:features") {
                break;
            }
            co_yield false;
        }
    }
    // auth
    {
        const auto auth = xmpp::elm::auth;
        conn->tx(xml::deparse(auth));
        co_yield true;

        const auto response = xml::parse(conn->rx_payload).as_value();
        DYN_ASSERT(response.name == "success");
    }
    // open
    {
        const auto open = xmpp::elm::open.clone().append_attrs({
            {"to", conn->host},
        });
        conn->tx(xml::deparse(open));
        co_yield true;

        const auto response = xml::parse(conn->rx_payload).as_value();
        DYN_ASSERT(response.name == "open");
    }
    // bind
    {
        const auto id = std::to_string((iq_serial += 1));

        const auto iq = xmpp::elm::iq.clone()
                            .append_attrs({
                                xml::Attribute{"id", id},
                                xml::Attribute{"type", "set"},
                            })
                            .append_children({
                                xmpp::elm::bind,
                            });
        conn->tx(xml::deparse(iq));
        co_yield true;

        while(true) {
            const auto response = xml::parse(conn->rx_payload).as_value();
            if(response.name != "iq") {
                co_yield false;
                continue;
            }
            DYN_ASSERT(response.is_attr_equal("id", id), "unexpected iq");
            const auto jid_n = response.find_first_child("bind", "jid");
            DYN_ASSERT(jid_n != nullptr, "jid not found in iq response");
            auto jid_o = Jid::parse(jid_n->data);
            DYN_ASSERT(jid_o.has_value());
            conn->jid = *std::move(jid_o);
            break;
        }
        if(config::debug_xmpp_connection) {
            print("jid: ", conn->jid.as_full());
        }
    }
    // disco
    {
        const auto id        = std::to_string((iq_serial += 1));
        const auto focus_jid = Jid{"focus", std::string("auth.") + conn->host, "focus"};

        const auto iq = xmpp::elm::iq.clone()
                            .append_attrs({
                                {"id", id},
                                {"type", "get"},
                                {"from", conn->jid.as_full()},
                                {"to", conn->host},
                            })
                            .append_children({
                                xmpp::elm::query,
                            });
        conn->tx(xml::deparse(iq));
        co_yield true;

        while(true) {
            const auto response = xml::parse(conn->rx_payload).as_value();
            if(response.name != "iq") {
                co_yield false;
                continue;
            }
            DYN_ASSERT(response.is_attr_equal("id", id), "unexpected iq");
            DYN_ASSERT(response.is_attr_equal("type", "result"), "unexpected iq");
            // TODO
            // parse disco
            break;
        }
    }
    // disco ext
    {
        const auto id = std::to_string((iq_serial += 1));

        const auto iq = xmpp::elm::iq.clone()
                            .append_attrs({
                                {"id", id},
                                {"type", "get"},
                                {"from", conn->jid.as_full()},
                                {"to", conn->host},
                            })
                            .append_children({
                                xmpp::elm::services,
                            });
        conn->tx(xml::deparse(iq));
        co_yield true;

        while(true) {
            const auto response = xml::parse(conn->rx_payload).as_value();
            if(response.name != "iq") {
                co_yield false;
                continue;
            }
            DYN_ASSERT(response.is_attr_equal("id", id), "unexpected iq");
            DYN_ASSERT(response.is_attr_equal("type", "result"), "unexpected iq");
            const auto services = response.find_first_child("services");
            DYN_ASSERT(services != nullptr);
            if(auto sv_o = parse_services(*services); sv_o) {
                conn->external_services = std::move(*sv_o);
            }
            break;
        }
    }
    co_return true;

    // DEBUG
    // idle
    co_yield true;
    while(true) {
        if(config::debug_xmpp_connection) {
            const auto response = xml::parse(conn->rx_payload).as_value();
            xml::dump_node(response);
        }
        co_yield true;
    }
    co_return true;
}
} // namespace

auto create(std::string host, std::function<void(std::string_view)> tx) -> Connection* {
    auto conn = new Connection{
        .host = std::move(host),
        .tx   = tx,
    };
    return conn;
}

auto finish(Connection* const conn) -> NegotiaionResult {
    const auto r = NegotiaionResult{
        .jid               = std::move(conn->jid),
        .external_services = std::move(conn->external_services),
    };
    delete conn;
    return r;
}

auto start_negotiation(Connection* const conn) -> void {
    conn->worker.start(negotiate, conn);
    conn->worker.resume();
}

auto resume_negotiation(Connection* const conn, const std::string_view payload) -> bool {
    conn->rx_payload = payload;
    conn->worker.resume();
    return conn->worker.done();
}
} // namespace xmpp
