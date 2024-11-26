#include "util/assert.hpp"
#include "xmpp/elements.hpp"

auto compute_disco_str(const xml::Node& disco) -> std::string {
    auto r = std::string();

    {
        const auto identity = disco.find_first_child(xmpp::elm::identity.name);
        dynamic_assert(identity != nullptr);

        auto category = std::string_view();
        auto type     = std::string_view();
        auto lang     = std::string_view();
        auto name     = std::string_view();
        dynamic_assert(identity->get_attrs({
            {"category", category},
            {"type", type},
            {"xml:lang", lang},
            {"name", name},
        }));
        r += build_string(category, "/", type, "/", lang, "/", name, "<");
    }
    // TODO: sort items
    for(const auto& c : disco.children) {
        if(c.name == "feature") {
            r += c.find_attr("var").value();
            r += "<";
        }
    }
    // TODO: append extensions
    return r;
}
