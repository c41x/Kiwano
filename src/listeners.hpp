#pragma once
#include "includes.hpp"

class mouseListener : public MouseListener {
protected:
	void call() {
		base::string lispArgs = "";
		for (auto &e : args)
			lispArgs += base::strs(" \"", e(), "\"");
		gl.eval(base::strs("(", functionId, lispArgs, ")"));
	}
public:
	mouseListener(base::lisp &interp, base::string fxId, Component *com) :
			functionId(fxId), gl(interp), c(com) {}
	base::string functionId;
	base::lisp &gl;
	Component *c;
	std::vector<std::function<base::string()>> args;
};

class mouseUpListener : public mouseListener {
public:
	mouseUpListener(base::lisp &interp, base::string fxId, Component *com) : mouseListener(interp, fxId, com) {}
	void mouseUp(const MouseEvent &e) override { call(); }
};

class mouseDoubleClickListener : public mouseListener {
public:
	mouseDoubleClickListener(base::lisp &interp, base::string fxId, Component *com) : mouseListener(interp, fxId, com) {}
	void mouseDoubleClick(const MouseEvent &e) override { call(); }
};

class mouseDownListener : public mouseListener {
public:
	mouseDownListener(base::lisp &interp, base::string fxId, Component *com) : mouseListener(interp, fxId, com) {}
	void mouseDown(const MouseEvent &e) override { call(); }
};
