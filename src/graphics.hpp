#pragma once
#include "includes.hpp"
#include "fxTemplates.hpp"

namespace graphics {

Graphics *g = nullptr;

base::cell_t drawText(base::lisp &gl, base::cell_t c, base::cells_t &) {
    return fxValidateSkeleton(gl, "g-draw-text", c, [&gl, c]() -> auto {
            const auto &str = c + 1;
            const auto &x = c + 2;
            const auto &y = c + 3;
            const auto &w = c + 4;
            const auto &h = c + 5;
            g->drawText(str->s, x->i, y->i, w->i, h->i, Justification::centredLeft, true);
            return gl.t();
        }, base::cell::list(5), base::cell::typeString,
        base::cell::typeInt, base::cell::typeInt, base::cell::typeInt, base::cell::typeInt);
}

}
