#pragma once
#include "includes.hpp"

// generic listener to pass events to lisp interpreter
class listener : public MouseListener,
				 public Slider::Listener,
				 public Timer,
				 public ModalComponentManager::Callback {
protected:
	void call() {
		base::string lispArgs = "";
		for (auto &e : args)
			lispArgs += e();
		gl.eval(base::strs("(", functionId, " ", lispArgs, ")"));
	}
public:
	listener(base::lisp &interp, base::string fxId, Component *com) :
			functionId(fxId), gl(interp), c(com) {}
	base::string functionId;
	base::lisp &gl;
	Component *c; // just to identyfy ownership
	std::vector<std::function<base::string()>> args;

	// these are pure virtual, so i must override them here
	void sliderValueChanged(Slider*) override {}
	void timerCallback() override {}
	void modalStateFinished(int) override {}
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
class sliderValueChangedListener : public listener {
public:
	sliderValueChangedListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void sliderValueChanged(Slider *) override { call(); }
};

class sliderDragBeginListener : public listener {
public:
	sliderDragBeginListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void sliderDragStarted(Slider *) override { call(); }
};

class sliderDragEndedListener : public listener {
public:
	sliderDragEndedListener(base::lisp &interp, base::string fxId, Component *com) : listener(interp, fxId, com) {}
	void sliderDragEnded(Slider *) override { call(); }
};

// Timer
class timerListener : public listener {
public:
	timerListener(base::lisp &interp, base::string fxId) : listener(interp, fxId, nullptr) {}
	void timerCallback() override { call(); }
};

// Async Alert window
class alertWindowListener : public listener, public AlertWindow {
public:
	alertWindowListener(base::lisp &interp, base::string fxId,
						std::string caption, std::string text)
			: listener(interp, fxId, nullptr),
			  AlertWindow(caption, text, AlertWindow::QuestionIcon) {
		args.push_back([this]() {
				return getTextEditorContents("text").toStdString();
			});
	}

	void modalStateFinished(int) override {
		call();
	}
};
