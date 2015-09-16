#pragma once
#include "includes.hpp"
#include "tabs.hpp"
#include "interpreter.hpp"
#include "layout.hpp"

class user_interface : public Component {
	std::map<std::string, std::unique_ptr<Component>> components;
	Component *mainComponent;
	base::lisp &gl;

public:
	user_interface(base::lisp &glisp) : mainComponent(nullptr), gl(glisp) {}
	~user_interface() {}

	void resized() override {
		if (mainComponent)
			mainComponent->setBounds(getLocalBounds());
	}

	// (set-main-component name)
	base::cell_t set_main_component(base::cell_t c, base::cells_t &ret) {
		const auto &name = c + 1;
		auto cc = components.find(name->s);
		if (cc != components.end()) {
			mainComponent = cc->second.get();
			addAndMakeVisible(mainComponent);
			mainComponent->setBounds(getLocalBounds());
		}
		return c;
	}

	// (refresh-interface)
	base::cell_t refresh_interface(base::cell_t c, base::cells_t &ret) {
		if (mainComponent)
			mainComponent->resized();
		return c;
	}

	// (create-playlist name)
	base::cell_t create_playlist(base::cell_t c, base::cells_t &ret) {
		// TODO: error reporting
		const auto &name = c + 1;
		if (components.find(name->s) == components.end()) {
			components.insert(std::make_pair(name->s, std::make_unique<playlist>()));
		}
		return c;
	}

	// (create-tabs name 'orientation{top, bottom, left, right})
	base::cell_t create_tabs(base::cell_t c, base::cells_t &ret) {
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
		}
		return c;
	}

	// (tabs-add-component tabs-name component-name caption |color|)
	base::cell_t tabs_add_component(base::cell_t c, base::cells_t &ret) {
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
			ptabs->addAndMakeVisible(com->second.get());
		}
		return c;
	}

	// (create-layout name (bool)horizontal (bool)splitter)
	base::cell_t create_layout(base::cell_t c, base::cells_t &ret) {
		const auto &name = c + 1;
		const auto &horizontal = c + 2;
		if (components.find(name->s) == components.end()) {
			components.insert(std::make_pair(name->s, std::make_unique<layout>(horizontal->s != "nil")));
		}
		return c;
		// TODO: returning quoted ID
	}

	// (create-interpreter name)
	base::cell_t create_interpreter(base::cell_t c, base::cells_t &ret) {
		const auto &name = c + 1;
		if (components.find(name->s) == components.end()) {
			components.insert(std::make_pair(name->s, std::make_unique<interpreter>(gl)));
		}
		return c;
	}

	// (layout-add-component layout-id component-id (float)min (float)max (float)preffered)
	base::cell_t layout_add_component(base::cell_t c, base::cells_t &ret) {
		// TODO: expected format: list of 5, id, id, float, float, float
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
		}
		return c;
	}

	// (layout-add-splitter layout-id)
	base::cell_t layout_add_splitter(base::cell_t c, base::cells_t &ret) {
		const auto &lname = c + 1;
		auto l = components.find(lname->s);
		if (l != components.end()) {
			layout *lay = reinterpret_cast<layout*>(l->second.get());
			lay->addSplitter();
		}
		return c;
	}
};
