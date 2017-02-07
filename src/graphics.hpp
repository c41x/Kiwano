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
            const auto &just = c + 6;
            g->drawText(str->s, x->i, y->i, w->i, h->i, just->i, true);
            return gl.t();
        }, base::cell::list(6), base::cell::typeString,
        base::cell::typeInt, base::cell::typeInt, base::cell::typeInt, base::cell::typeInt,
        base::cell::typeInt);
}

base::cell_t setColor(base::lisp &gl, base::cell_t c, base::cells_t &) {
    return fxValidateSkeleton(gl, "g-set-color", c, [&gl, c]() -> auto {
            const auto &color = c + 1;
            g->setColour(Colour((uint8)(color->v4[0] * 255.0f),
                                (uint8)(color->v4[1] * 255.0f),
                                (uint8)(color->v4[2] * 255.0f),
                                (uint8)(color->v4[3] * 255.0f)));
            return gl.t();
        }, base::cell::list(1), base::cell::typeVector);
}

}
