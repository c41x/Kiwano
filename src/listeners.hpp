#pragma once
#include "includes.hpp"

// TODO: change to MouseListener? all Component instances have mouse listener
// example: (component-bind-callback 'some-component 'callback-fx)
// class listener : public Button::Listener {
// 	base::string functionId;
// 	base::lisp &gl;
// public:
// 	listener(base::lisp &interp, base::string fxId) : functionId(fxId), gl(interp) {}
// 	void buttonClicked(Button *) override { gl.eval(base::strs("(", functionId, ")")); }
// };

// class playlistItemClickListener : public MouseListener {
// 	base::string functionId;
// 	base::lisp &gl;
// 	playlist *p;
// public:
// 	playlistItemClickListener(base::lisp &interp, base::string fxId, playlist *pl) : functionId(fxId), gl(interp), p(pl) {}
// 	void mouseDoubleClick(const MouseEvent &) { gl.eval(base::strs("(", functionId, " \"", p->getSelectedRowString(),"\"", ")")); }
// };

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
	void mouseUp(const MouseEvent &e) { call(); }
};

class mouseDoubleClickListener : public mouseListener {
public:
	mouseDoubleClickListener(base::lisp &interp, base::string fxId, Component *com) : mouseListener(interp, fxId, com) {}
	void mouseDoubleClick(const MouseEvent &e) { call(); }
};
