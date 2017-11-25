#pragma once
#include "includes.hpp"
#include "graphics.hpp"

class panel : public Component {
    base::lisp &glisp;
    base::string paintFunction;

public:
    panel(base::lisp &gl, const base::string &paintFn) :
            glisp(gl),
            paintFunction(paintFn) {}

    void paint(Graphics &g) override {
        if (graphics::gEnableCustomDrawing) {
            graphics::g = &g;
            glisp.eval(base::strs("(", paintFunction, ")"));
            graphics::g = nullptr;
        }
    }
};
