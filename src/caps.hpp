#pragma once
#include "xml/xml.hpp"

auto compute_disco_str(const xml::Node& disco) -> std::optional<std::string>;
