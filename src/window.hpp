#pragma once
#include "includes.hpp"

class window : public DocumentWindow {
	base::lisp &gl;
public:
	window(base::lisp &gli, const String &caption, Colour color, int buttons)
			: DocumentWindow(caption, color, buttons), gl(gli) {}

	void closeButtonPressed() override {
		// TODO: unregister
		//delete this;
	}
};
