#pragma once
#include "includes.hpp"
#include "tabs.hpp"
#include "interpreter.hpp"
#include "layout.hpp"
#include "playback.hpp"
#include "listeners.hpp"

class user_interface : public Component {
	std::map<std::string, std::unique_ptr<Component>> components;
	std::vector<std::unique_ptr<mouseListener>> listeners;
	Component *mainComponent;
	base::lisp &gl;

public:
	user_interface(base::lisp &glisp) : mainComponent(nullptr), gl(glisp) { }
	~user_interface() {}

	void resized() override {
		if (mainComponent)
			mainComponent->setBounds(getLocalBounds());
	}

	// (set-main-component name) -> bool
	base::cell_t set_main_component(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			auto cc = components.find(name->s);
			if (cc != components.end()) {
				mainComponent = cc->second.get();
				addAndMakeVisible(mainComponent);
				mainComponent->setBounds(getLocalBounds());
				return gl.t();
			}
			gl.signalError("component not found");
			return gl.nil();
		}
		gl.signalError("set-main-component: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (refresh-interface)
	base::cell_t refresh_interface(base::cell_t, base::cells_t &) {
		// signal to all components resized event (to refresh UI)
		for (auto &c : components)
			c.second->resized();
		return gl.t();
	}

	// (create-playlist name) -> nil/id
	base::cell_t create_playlist(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			if (components.find(name->s) == components.end()) {
				Component *com = components.insert(std::make_pair(name->s, std::make_unique<playlist>())).first->second.get();

				// TODO: test - remove
				// TODO: listeners with arguments binding
				listeners.push_back(std::make_unique<mouseDoubleClickListener>(gl, "on-playlist-click", com));
				listeners.back()->args.push_back(std::bind(&playlist::getSelectedRowString, reinterpret_cast<playlist*>(com)));
				com->addMouseListener(listeners.back().get(), true);

				return name;
			}
			gl.signalError(base::strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-playlist: invalid arguments, expected (id)");
		return gl.nil();
	}

	// TODO: removing callbacks
	// TODO: other callbacks
	// (bind-mouse-* (id)component (id)function) -> bool
	template <typename T>
	base::cell_t bind_mouse_listener(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeIdentifier,
								 base::cell::typeIdentifier)) {
			const auto &cname = c + 1;
			const auto &bname = c + 2;
			auto e = components.find(cname->s);
			if (e != components.end()) {
				Component *com = e->second.get();
				listeners.push_back(std::make_unique<T>(gl, bname->s, com));
				com->addMouseListener(listeners.back().get(), true);
				return gl.t();
			}
		}
		gl.signalError("bind-mouse-*: invalid arguments, expected (id id)");
		return gl.nil();
	}

	// (unbind-mouse (id)component (id)function)
	base::cell_t unbind_mouse_listener(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeIdentifier,
								 base::cell::typeIdentifier)) {
			const auto &cname = c + 1;
			const auto &bname = c + 2;
			auto e = components.find(cname->s);
			if (e != components.end()) {
				Component *com = e->second.get();
				auto l = std::find_if(listeners.begin(), listeners.end(), [&com, &bname](const auto &a) {
						return a->c == com && a->functionId == bname->s;
					});
				if (l != listeners.end()) {
					com->removeMouseListener((*l).get());
					listeners.erase(l);
					return gl.t();
				}
				return gl.nil();
			}
		}
		gl.signalError("unbind-mouse-*: invalid arguments, expected (id id)");
		return gl.nil();
	}

	base::cell_t playlist_get_selected(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				auto p = reinterpret_cast<playlist*>(e->second.get());
				ret.push_back(base::cell(base::cell::typeString, p->getSelectedRowString()));
				return ret.end();
			}
			gl.signalError(base::strs("component named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("playlist-get-selected: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (create-text-button name (string)caption (string)tooltip) -> nil/id
	base::cell_t create_text_button(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(3), cell::typeIdentifier, cell::typeString, cell::typeString)) {
			const auto &name = c + 1;
			const auto &label = c + 2;
			const auto &tip = c + 3;
			if (components.find(name->s) == components.end()) {
				components.insert(std::make_pair(name->s, std::make_unique<TextButton>(label->s, tip->s)));
				return name;
			}
			gl.signalError(strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-text-button: invalid arguments, expected (id string string)");
		return gl.nil();
	}

	// (create-tabs name 'orientation{top, bottom, left, right}) -> bool/id
	base::cell_t create_tabs(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(2), cell::typeIdentifier, cell::typeIdentifier)) {
			const auto &name = c + 1;
			if (components.find(name->s) == components.end()) {
				const auto &orientation = c + 2;
				TabbedButtonBar::Orientation o = TabbedButtonBar::TabsAtTop;
				if (orientation->s == "bottom")
					o = TabbedButtonBar::TabsAtBottom;
				else if (orientation->s == "left")
					o = TabbedButtonBar::TabsAtLeft;
				else if (orientation->s == "right")
					o = TabbedButtonBar::TabsAtRight;
				components.insert(std::make_pair(name->s, std::make_unique<tabs>(o)));
				return name;
			}
			gl.signalError(base::strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-tabs: invalid arguments, expected (id 'id)");
		return gl.nil();
	}

	// (create-audio-settings name)
	base::cell_t create_audio_settings(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(1), cell::typeIdentifier)) {
			const auto &name = c + 1;
			if (components.find(name->s) == components.end()) {
				components.insert(std::make_pair(name->s, std::make_unique<AudioDeviceSelectorComponent>(
													 playback::dm, 0, 256, 0, 256, false, false, true, false)));
				return gl.t();
			}
			gl.signalError(strs("components named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-audio-settings: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (tabs-add-component tabs-name component-name "caption" |color|) -> bool
	base::cell_t tabs_add_component(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(4), cell::typeIdentifier, cell::typeIdentifier, cell::typeString, cell::typeVector)) {
			const auto &name = c + 1;
			const auto &cname = c + 2;
			auto t = components.find(name->s);
			auto com = components.find(cname->s);
			if (t != components.end() && com != components.end()) {
				const auto &caption = c + 3;
				const auto &color = c + 4;
				tabs *ptabs = reinterpret_cast<tabs*>(t->second.get());
				ptabs->addTab(caption->s, Colour::fromFloatRGBA(color->v4[0],
																color->v4[1],
																color->v4[2],
																color->v4[3]), com->second.get(), false);
				return gl.t();
			}
			gl.signalError("tabs or component not found");
			return gl.nil();
		}
		gl.signalError("tabs-add-component: invalid arguments, expected (id id \"string\" |vector|)");
		return gl.nil();
	}

	// (create-layout name (bool)horizontal) -> bool/id
	base::cell_t create_layout(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(2), cell::typeIdentifier, cell::typeIdentifier)) {
			const auto &name = c + 1;
			const auto &horizontal = c + 2;
			if (components.find(name->s) == components.end()) {
				components.insert(std::make_pair(name->s, std::make_unique<layout>(!horizontal->isNil())));
				return name;
			}
			gl.signalError(strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-layout: invalid arguments, expected (id bool)");
		return gl.nil();
	}

	// (create-interpreter name) -> bool/id
	base::cell_t create_interpreter(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			if (components.find(name->s) == components.end()) {
				components.insert(std::make_pair(name->s, std::make_unique<interpreter>(gl)));
				return name;
			}
			gl.signalError(base::strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-interpreter: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (layout-add-component layout-id component-id (float)min (float)max (float)preffered) -> bool
	base::cell_t layout_add_component(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(5),
						   cell::typeIdentifier, cell::typeIdentifier,
						   cell::typeFloat, cell::typeFloat, cell::typeFloat)) {
			const auto &lname = c + 1;
			const auto &cname = c + 2;
			auto l = components.find(lname->s);
			auto com = components.find(cname->s);
			if (l != components.end() && com != components.end()) {
				const auto &minimum = c + 3;
				const auto &maximum = c + 4;
				const auto &preferred = c + 5;
				layout *lay = reinterpret_cast<layout*>(l->second.get());
				lay->addComponent(com->second.get(), (double)minimum->f, (double)maximum->f, (double)preferred->f);
				return gl.t();
			}
			gl.signalError("layout or component not found");
			return gl.nil();
		}
		gl.signalError("layout-add-component: invalid arguments, expected (id, id, float, float, float)");
		return gl.nil();
	}

	// (layout-remove-component layout-id component-id) -> bool
	base::cell_t layout_remove_component(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(2), cell::typeIdentifier, cell::typeIdentifier)) {
			const auto &lname = c + 1;
			const auto &cname = c + 2;
			auto l = components.find(lname->s);
			auto com = components.find(cname->s);
			if (l != components.end() && com != components.end()) {
				layout *lay = reinterpret_cast<layout*>(l->second.get());
				lay->removeComponent(com->second.get());
				return gl.t();
			}
			gl.signalError("layout or component not found");
			return gl.nil();
		}
		gl.signalError("layout-remove-component: invalid arguments, expected (id id)");
		return gl.nil();
	}

	// (layout-remove-splitter layout-id (int)splitter-index)
	base::cell_t layout_remove_splitter(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(2), cell::typeIdentifier, cell::typeInt)) {
			const auto &lname = c + 1;
			auto l = components.find(lname->s);
			if (l != components.end()) {
				const auto sIndex = c + 2;
				layout *lay = reinterpret_cast<layout*>(l->second.get());
				lay->removeSplitter(sIndex->i);
				return gl.t();
			}
			gl.signalError("layout not found");
			return gl.nil();
		}
		gl.signalError("layout-remove-splitter: invalid arguments, expected (id int)");
		return gl.nil();
	}

	// (layout-add-splitter layout-id)
	base::cell_t layout_add_splitter(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &lname = c + 1;
			auto l = components.find(lname->s);
			if (l != components.end()) {
				layout *lay = reinterpret_cast<layout*>(l->second.get());
				lay->addSplitter();
				return gl.t();
			}
			gl.signalError("layout not found");
			return gl.nil();
		}
		gl.signalError("layout-add-splitter: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (layout-get-splitters-count layout-id)
	base::cell_t layout_get_splitters_count(base::cell_t c, base::cells_t &ret) {
		using namespace base;
		if (lisp::validate(c, cell::list(1), cell::typeIdentifier)) {
			const auto &lname = c + 1;
			auto l = components.find(lname->s);
			if (l != components.end()) {
				layout *lay = reinterpret_cast<layout*>(l->second.get());
				ret.push_back(base::cell(lay->getSplittersCount()));
				return ret.end();
			}
			gl.signalError("layout not found");
			return gl.nil();
		}
		gl.signalError("layout-get-splitters-count: invalid arguments, expected (id)");
		return gl.nil();
	}
};
