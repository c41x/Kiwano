#pragma once
#include "includes.hpp"

// basic call with validation
template <typename... Args, typename T>
granite::base::cell_t fxValidateSkeleton(granite::base::lisp &gl, const granite::base::string &fxName,
										 granite::base::cell_t c, T fx, Args... v) {
	using namespace granite::base;
	if (lisp::validate(c, v...))
		return fx();
	gl.signalError(strs(fxName, ": invalid arguments, expected (", lisp::validateStr(v...), ")"));
	return gl.nil();
}

// validate + call only if element is not present in container
template <typename... Args, typename T, typename TC>
granite::base::cell_t fxValidateAccessSkeleton(granite::base::lisp &gl, const granite::base::string &fxName,
											   granite::base::cell_t c, TC fx, T &container, Args... v) {
	using namespace granite::base;
	if (lisp::validate(c, v...)) {
		const auto &name = c + 1;
		auto cc = container.find(name->s);
		if (cc != container.end()) {
			return fx(cc->second.get());
		}
		gl.signalError(strs(fxName, ": component named \"", name->s, "\" not found"));
		return gl.nil();
	}
	gl.signalError(strs(fxName, ": invalid arguments, expected (", lisp::validateStr(v...), ")"));
	return gl.nil();
}

// validate + call only if element is not present in container (no error when not found)
template <typename... Args, typename T, typename TC>
granite::base::cell_t fxValidateTryAccessSkeleton(granite::base::lisp &gl, const granite::base::string &fxName,
												  granite::base::cell_t c, TC fx, T &container, Args... v) {
	using namespace granite::base;
	if (lisp::validate(c, v...)) {
		const auto &name = c + 1;
		auto cc = container.find(name->s);
		if (cc != container.end()) {
			return fx(cc->second.get());
		}
		return gl.nil();
	}
	gl.signalError(strs(fxName, ": invalid arguments, expected (", lisp::validateStr(v...), ")"));
	return gl.nil();
}

// validate + call only if element present in container
template <typename... Args, typename T, typename TC>
granite::base::cell_t fxValidateCreateSkeleton(granite::base::lisp &gl, const granite::base::string &fxName,
											   granite::base::cell_t c, TC fx, T &container, Args... v) {
	using namespace granite::base;
	if (lisp::validate(c, v...)) {
		const auto &name = c + 1;
		auto cc = container.find(name->s);
		if (cc == container.end()) {
			return fx();
		}
		gl.signalError(strs(fxName, ": component named \"", name->s, "\" already exists"));
		return gl.nil();
	}
	gl.signalError(strs(fxName, ": invalid arguments, expected (", lisp::validateStr(v...), ")"));
	return gl.nil();
}

// TODO: use get()?
