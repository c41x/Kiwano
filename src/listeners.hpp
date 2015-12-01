#pragma once
#include "includes.hpp"

// generic listener to pass events to lisp interpreter
class listener : public MouseListener {
protected:
	void call() {
		base::string lispArgs = "";
		for (auto &e : args)
			lispArgs += base::strs(" \"", e(), "\"");
		gl.eval(base::strs("(", functionId, lispArgs, ")"));
	}
public:
	listener(base::lisp &interp, base::string fxId, Component *com) :
			functionId(fxId), gl(interp), c(com) {}
	base::string functionId;
	base::lisp &gl;
	Component *c;
	std::vector<std::function<base::string()>> args;
};

// mouse listeners for Component class
class mouseUpListener : public listener {
public:
	mouseUpListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void mouseUp(const MouseEvent &e) override { call(); }
};

class mouseDoubleClickListener : public listener {
public:
	mouseDoubleClickListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void mouseDoubleClick(const MouseEvent &e) override { call(); }
};

class mouseDownListener : public listener {
public:
	mouseDownListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void mouseDown(const MouseEvent &e) override { call(); }
};

// Slider listeners
class sliderValueChangedListener : public Slider::Listener, listener {
public:
	sliderValueChangedListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void sliderValueChanged(Slider *) override { call(); }
};

class sliderDragBeginListener : public Slider::Listener, listener {
public:
	sliderDragBeginListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void sliderDragStarted(Slider *) override { call(); }
};

class sliderDragEndedListener : public Slider::Listener, listener {
public:
	sliderDragEndedListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void sliderDragEnded(Slider *) override { call(); }
};
