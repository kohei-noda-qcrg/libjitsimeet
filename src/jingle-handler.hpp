#pragma once
#include "jingle/jingle.hpp"

class JingleHandler {
  public:
    virtual auto initiate(jingle::Jingle /*jingle*/) -> bool {
        return true;
    };

    virtual auto add_source(jingle::Jingle /*jingle*/) -> bool {
        return true;
    };

    virtual ~JingleHandler(){};
};
