#pragma once
#include "includes.hpp"
#include "tabs.hpp"
#include "interpreter.hpp"
#include "layout.hpp"
#include "playback.hpp"
#include "listeners.hpp"
#include "slider.hpp"

// change listener
class audioSettingsChangeListener : public ChangeListener {
public:
	AudioDeviceManager *am;
	audioSettingsChangeListener() : am(nullptr) {}
	void changeListenerCallback(ChangeBroadcaster *source) override {
		if (am == source && am) {
			//std::cout << "store audio settings" << std::endl; TODO: log this
			if (am->createStateXml()) {
				//std::cout << "state xml created | saving fo file" << std::endl;
				base::fs::store("audio-settings.xml", base::fromStr<base::stream>(
									am->createStateXml()->createDocument("").toRawUTF8()));
			}
		}
	}
};

class user_interface : public Component {
	std::map<base::string, std::unique_ptr<Component>> components;
	std::map<base::string, std::unique_ptr<timerListener>> timers;
	std::vector<std::unique_ptr<listener>> listeners;
	Component *mainComponent;
	base::lisp &gl;
	const base::string idAudioSettings;
	std::unique_ptr<audioSettingsChangeListener> asListener;

public:
	user_interface(base::lisp &glisp) : mainComponent(nullptr), gl(glisp), idAudioSettings("#audio-settings#"),
										asListener(std::make_unique<audioSettingsChangeListener>()){ }
	~user_interface() {}

	void resized() override {
		if (mainComponent)
			mainComponent->setBounds(getLocalBounds());
	}

	//- general GUI
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

	// (has-component name) -> bool
	base::cell_t has_component(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			auto cc = components.find(name->s);
			if (cc != components.end())
				return gl.t();
			return gl.nil();
		}
		gl.signalError("has-component: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (repaint-component name) -> bool
	base::cell_t repaint_component(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			auto cc = components.find(name->s);
			if (cc != components.end()) {
				cc->second->repaint();
				return gl.t();
			}
			return gl.nil();
		}
		gl.signalError("repaint-component: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (get-components (id)type) -> (list of id)
	base::cell_t get_components(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &type = c + 1;
			ret.push_back(base::cell::list(0));
			auto &head = ret.back();
			for (const auto &com : components) {
				if (com.second->getName() == type->s) {
					ret.push_back(base::cell(base::cell::typeIdentifier, com.first));
					head.i++;
				}
			}
			return ret.end();
		}
		gl.signalError("get-components: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (get-child-components (id)type) -> (list of id)
	base::cell_t get_child_components(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeIdentifier,
								 base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			const auto &type = c + 2;
			auto e = components.find(name->s);
			if (e != components.end()) {
				ret.push_back(base::cell::list(0));
				auto &head = ret.back();
				auto &com = e->second;

				auto addToList = [&ret, &head](Component *c) {
					ret.push_back(base::cell(base::cell::typeIdentifier,
											 c->getComponentID().toStdString()));
					head.i++;
				};

				// if component is 'tabs iterate through it's items
				if (com->getName() == "tabs") {
					tabs *t = reinterpret_cast<tabs*>(com.get());
					for (int i = 0; i < t->getNumTabs(); ++i) {
						auto tt = t->getTabContentComponent(i);
						if (tt->getName() == type->s) {
							addToList(tt);
						}
					}
				}
				else {
					// default search in JUCE hierarchy
					for (int i = 0; i < com->getNumChildComponents(); ++i) {
						Component *ccom = com->getChildComponent(i);
						if (ccom->getName() == type->s) {
							addToList(ccom);
						}
					}
				}
				return ret.end();
			}
			gl.signalError(base::strs("component named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("get-child-components: invalid arguments, expected (id id)");
		return gl.nil();
	}

	// (component-enabled name (optional|nil/t)) -> bool
	base::cell_t component_enabled(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::listRange(1, 2),
								 base::cell::typeIdentifier,
								 base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			auto cc = components.find(name->s);
			if (cc != components.end()) {
				if (c->listSize() == 1) {
					// getter
					if (cc->second->isEnabled())
						return gl.t();
					return gl.nil();
				}

				// setter
				const auto &doEnable = c + 2;
				cc->second->setEnabled(!doEnable->isNil());
				return doEnable;
			}
			gl.signalError(base::strs("component-enabled: component named \"", name->s, "\" not found"));
			return gl.nil();
		}
		gl.signalError("component-enabled: invalid arguments, expected (id (optional)nil/t)");
		return gl.nil();
	}

	// (refresh-interface)
	base::cell_t refresh_interface(base::cell_t, base::cells_t &) {
		// signal to all components resized event (to refresh UI)
		for (auto &c : components)
			c.second->resized();
		return gl.t();
	}

	// (unique-id (string)prefix) -> string / nil
	base::cell_t unique_id(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeString)) {
			const auto &prefix = c + 1;
			base::string gid;
			for (int32 n = 0; n < 99999; ++n) {
				for (const auto &com : components) {
					gid = base::strs(prefix->s, "_", n);
					if (gid != com.first) {
						ret.push_back(gid);
						return ret.end();
					}
				}
			}
			return gl.nil();
		}
		gl.signalError("unique-id: invalid arguments, expected (string)");
		return gl.nil();
	}

	//- playlist
	// (create-playlist name) -> nil/id
	base::cell_t create_playlist(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			if (components.find(name->s) == components.end()) {
				auto &p = components.insert(std::make_pair(name->s, std::make_unique<playlist>())).first->second;
				p->setName("playlist");
				p->setComponentID(name->s);
				return name;
			}
			gl.signalError(base::strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-playlist: invalid arguments, expected (id)");
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

	base::cell_t playlist_items_count(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				auto p = reinterpret_cast<playlist*>(e->second.get());
				ret.push_back(base::cell(base::cell::typeInt, p->getItemsCount()));
				return ret.end();
			}
			gl.signalError(base::strs("component named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("playlist-items-count: invalid arguments, expected (id)");
		return gl.nil();
	}

	base::cell_t playlist_get(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(3), base::cell::typeIdentifier,
								 base::cell::typeInt, base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			const auto &index = c + 2;
			const auto &query = c + 3;
			auto e = components.find(name->s);
			if (e != components.end()) {
				auto p = reinterpret_cast<playlist*>(e->second.get());
				if (query->s == "id")
					ret.push_back(base::cell(base::cell::typeString, p->getRowId(index->i)));
				else if (query->s == "path")
					ret.push_back(base::cell(base::cell::typeString, p->getRowPath(index->i)));
				else return gl.nil();
				return ret.end();
			}
			gl.signalError(base::strs("component named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("playlist-get-selected: invalid arguments, expected (id)");
		return gl.nil();
	}

	base::cell_t playlist_load(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeString)) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				const auto &path = c + 2;
				auto p = reinterpret_cast<playlist*>(e->second.get());
				if (p->load(path->s))
					return gl.t();
				return gl.nil();
			}
			gl.signalError(base::strs("component named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("playlist-load: invalid arguments, expected (id string)");
		return gl.nil();
	}

	// TODO: do not duplicate?
	base::cell_t playlist_save(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeString)) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				const auto &path = c + 2;
				auto p = reinterpret_cast<playlist*>(e->second.get());
				if (p->store(path->s))
					return gl.t();
				return gl.nil();
			}
			gl.signalError(base::strs("component named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("playlist-save: invalid arguments, expected (id string)");
		return gl.nil();
	}

	// (playlist-add-column (id)playlist (string)caption (id)content (int)width (int)minWidth (int)maxWidth)
	base::cell_t playlist_add_column(base::cell_t c, base::cells_t &ret) {
		using namespace base;
		if (lisp::validate(c, cell::list(6), cell::typeIdentifier, cell::typeString, cell::typeIdentifier,
						   cell::typeInt, cell::typeInt, cell::typeInt)) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				const auto &caption = c + 2;
				const auto &content = c + 3;
				const auto &width = c + 4;
				const auto &minWidth = c + 5;
				const auto &maxWidth = c + 6;
				auto p = reinterpret_cast<playlist*>(e->second.get());
				p->addColumn(caption->s, content->s, width->i, minWidth->i, maxWidth->i);
				return gl.t();
			}
			gl.signalError(strs("component named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("playlist-add-column: invalid arguments, expected (id string id int int int)");
		return gl.nil();
	}

	//- text button
	// (create-text-button name (string)caption (string)tooltip) -> nil/id
	base::cell_t create_text_button(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(3), cell::typeIdentifier, cell::typeString, cell::typeString)) {
			const auto &name = c + 1;
			const auto &label = c + 2;
			const auto &tip = c + 3;
			if (components.find(name->s) == components.end()) {
				auto &p = components.insert(std::make_pair(name->s, std::make_unique<TextButton>(label->s, tip->s))).first->second;
				p->setName("text-button");
				p->setComponentID(name->s);
				return name;
			}
			gl.signalError(strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-text-button: invalid arguments, expected (id string string)");
		return gl.nil();
	}

	//- audio settings
	// (audio-settings) -> name
	base::cell_t audio_settings(base::cell_t, base::cells_t &ret) {
		using namespace base;
		if (components.find(idAudioSettings) == components.end()) {
			Component *com = components.insert(std::make_pair(idAudioSettings, std::make_unique<AudioDeviceSelectorComponent>(
																  playback::dm, 0, 256, 0, 256, false, false, true, false))).first->second.get();
			AudioDeviceSelectorComponent *ac = (AudioDeviceSelectorComponent*)com;
			if (fs::exists("audio-settings.xml")) {
				XmlDocument doc(toStr(fs::load("audio-settings.xml")));
				ac->deviceManager.initialise(0, 2, doc.getDocumentElement(), true);
			}
			else {
				ac->deviceManager.initialiseWithDefaultDevices(0, 2);
			}
			asListener->am = &ac->deviceManager;
			ac->deviceManager.addChangeListener(asListener.get());
		}
		ret.push_back(cell(cell::typeIdentifier, idAudioSettings));
		return ret.end();
	}

	//- interpreter
	// (create-interpreter name) -> bool/id
	base::cell_t create_interpreter(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &name = c + 1;
			if (components.find(name->s) == components.end()) {
				auto &p = components.insert(std::make_pair(name->s, std::make_unique<interpreter>(gl))).first->second;
				p->setName("interpreter");
				p->setComponentID(name->s);
				return name;
			}
			gl.signalError(base::strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-interpreter: invalid arguments, expected (id)");
		return gl.nil();
	}

	//- layout
	// (create-layout name (bool)horizontal) -> bool/id
	base::cell_t create_layout(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(2), cell::typeIdentifier, cell::typeIdentifier)) {
			const auto &name = c + 1;
			const auto &horizontal = c + 2;
			if (components.find(name->s) == components.end()) {
				auto &p = components.insert(std::make_pair(name->s, std::make_unique<layout>(!horizontal->isNil()))).first->second;
				p->setName("layout");
				p->setComponentID(name->s);
				return name;
			}
			gl.signalError(strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-layout: invalid arguments, expected (id bool)");
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

	//- tabs
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
				auto &p = components.insert(std::make_pair(name->s, std::make_unique<tabs>(o))).first->second;
				p->setName("tabs");
				p->setComponentID(name->s);
				return name;
			}
			gl.signalError(base::strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-tabs: invalid arguments, expected (id 'id)");
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

	// (tabs-index (id)tabs (int|string|optional)index)
	base::cell_t tabs_index(base::cell_t c, base::cells_t &ret) {
		using namespace base;
		if (lisp::validate(c, cell::listRange(1, 2), cell::typeIdentifier, cell::anyOf(cell::typeInt, cell::typeString))) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				tabs *t = reinterpret_cast<tabs*>(e->second.get());

				// getter
				if (c->listSize() == 1) {
					ret.push_back(cell((int32)t->getCurrentTabIndex()));
					return ret.end();
				}

				// setter
				const auto &index = c + 2;
				if (index->type == cell::typeInt) {
					t->setCurrentTabIndex(index->i); // for int
					return index;
				}

				// for string
				int ii = t->selectTabByName(index->s);
				if (ii != -1) {
					ret.push_back(cell(ii));
					return ret.end();
				}
				return gl.nil();
			}
			gl.signalError(strs("tabs named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("tabs-index: invalid arguments, expected (id (optional)int)");
		return gl.nil();
	}

	// (tabs-count (id)tabs)
	base::cell_t tabs_count(base::cell_t c, base::cells_t &ret) {
		using namespace base;
		if (lisp::validate(c, cell::list(1), cell::typeIdentifier)) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				tabs *t = reinterpret_cast<tabs*>(e->second.get());
				ret.push_back(cell((int32)t->getNumTabs()));
				return ret.end();
			}
			gl.signalError(strs("tabs named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("tabs-count: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (tabs-get-components (id)tabs (id)type)
	base::cell_t tabs_get_components(base::cell_t c, base::cells_t &ret) {
			using namespace base;
			if (lisp::validate(c, cell::list(2), cell::typeIdentifier, cell::typeIdentifier)) {
				const auto &name = c + 1;
				const auto &type = c + 2;
				auto e = components.find(name->s);
				if (e != components.end()) {
					tabs *t = reinterpret_cast<tabs*>(e->second.get());
					ret.push_back(cell::list(0));
					auto &head = ret.back();
					auto tabNames = t->getTabNames();
					for (int i = 0; i < t->getNumTabs(); ++i) {
						auto tt = t->getTabContentComponent(i);
						if (tt->getName() == type->s) {
							ret.push_back(cell::list(2));
							ret.push_back(cell(cell::typeIdentifier, tt->getComponentID().toStdString()));
							ret.push_back(cell(cell::typeString, tabNames[i].toStdString()));
							head.i++;
						}
					}
					return ret.end();

				}
				gl.signalError(strs("tabs named: ", name->s, " not found"));
				return gl.nil();
			}
			gl.signalError("tabs-get-components: invalid arguments, expected (id id)");
			return gl.nil();
	}

	// (tabs-remove (id)tabs (int|string)index)
	base::cell_t tabs_remove(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::list(2), cell::typeIdentifier, cell::anyOf(cell::typeInt, cell::typeString))) {
			const auto &name = c + 1;
			auto e = components.find(name->s);
			if (e != components.end()) {
				tabs *t = reinterpret_cast<tabs*>(e->second.get());
				const auto &index = c + 2;
				if (index->type == cell::typeInt)
					t->removeTabByIndex(index->i);
				else t->removeTabByName(index->s);
				return gl.t();
			}
			gl.signalError(strs("tabs named: ", name->s, " not found"));
			return gl.nil();
		}
		gl.signalError("tabs-remove: invalid arguments, expected (id int)");
		return gl.nil();
	}

	//- slider
	// (create-slider name 'style 'edit-box) -> nil/id
	base::cell_t create_slider(base::cell_t c, base::cells_t &) {
		using namespace base;
		if (lisp::validate(c, cell::listRange(1, 3), cell::typeIdentifier, cell::typeIdentifier, cell::typeIdentifier)) {
			const auto &name = c + 1;
			if (components.find(name->s) == components.end()) {
				slider::SliderStyle style = slider::LinearHorizontal;
				slider::TextEntryBoxPosition editBox = slider::NoTextBox;
				if (c->listSize() > 1) {
					const auto &s = c + 2;
					if (s->s == "linear-horizontal") style = slider::LinearHorizontal;
					else if (s->s == "linear-vertical") style = slider::LinearVertical;
					else if (s->s == "linear-bar") style = slider::LinearBar;
					else if (s->s == "linear-bar-vertical") style = slider::LinearBarVertical;
					else if (s->s == "rotary") style = slider::Rotary;
					else if (s->s == "rotary-horizontal-drag") style = slider::RotaryHorizontalDrag;
					else if (s->s == "rotary-vertical-drag") style = slider::RotaryVerticalDrag;
					else if (s->s == "rotary-horizontal-vertical-drag") style = slider::RotaryHorizontalVerticalDrag;
					else if (s->s == "inc-dec-buttons") style = slider::IncDecButtons;
					else if (s->s == "two-value-horizontal") style = slider::TwoValueHorizontal;
					else if (s->s == "two-value-vertical") style = slider::TwoValueVertical;
					else if (s->s == "three-value-horizontal") style = slider::ThreeValueHorizontal;
					else if (s->s == "three-value-vertical") style = slider::ThreeValueVertical;
				}
				if (c->listSize() > 2) {
					const auto &e = c + 3;
					if (e->s == "no-text-box") editBox = slider::NoTextBox;
					else if (e->s == "tex-box-left") editBox = slider::TextBoxLeft;
					else if (e->s == "tex-box-right") editBox = slider::TextBoxRight;
					else if (e->s == "tex-box-above") editBox = slider::TextBoxAbove;
					else if (e->s == "tex-box-below") editBox = slider::TextBoxBelow;
				}
				auto &p = components.insert(std::make_pair(name->s, std::make_unique<slider>(style, editBox))).first->second;
				p->setName("slider");
				p->setComponentID(name->s);
				return name;
			}
			gl.signalError(strs("component named ", name->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-slider: invalid arguments, expected (id (id|optional) (id|optional))");
		return gl.nil();
	}

	// (slider-range (id)slider-id (float|optional)range-min (float|optional)range-max) -> nil/t | (float float)value
	base::cell_t slider_range(base::cell_t c, base::cells_t &ret) {
		using namespace base;
		bool isSetter = lisp::validate(c, cell::list(3), cell::typeIdentifier, cell::typeFloat, cell::typeFloat);
		bool isGetter = lisp::validate(c, cell::list(1), cell::typeIdentifier);
		if (isGetter || isSetter) {
			const auto &lname = c + 1;
			auto l = components.find(lname->s);
			if (l != components.end()) {
				slider *sl = reinterpret_cast<slider*>(l->second.get());
				if (isSetter) {
					sl->setRange((double)(c + 2)->f,
								 (double)(c + 3)->f);
					return gl.t();
				}

				// getter (returns list tuple with minumum and maximum values)
				ret.push_back(cell::list(2));
				ret.push_back(cell((float)sl->getMinimum()));
				ret.push_back(cell((float)sl->getMaximum()));
				return ret.end();
			}
			gl.signalError("slider not found");
			return gl.nil();
		}
		gl.signalError("slider-set-range: invalid arguments, expected (id float float)");
		return gl.nil();
	}

	// (slider-value (id)slider-id (float|optional)value) -> nil/t | (float)value
	base::cell_t slider_value(base::cell_t c, base::cells_t &ret) {
		using namespace base;
		if (lisp::validate(c, cell::listRange(1, 2), cell::typeIdentifier, cell::typeFloat)) {
			const auto &lname = c + 1;
			auto l = components.find(lname->s);
			if (l != components.end()) {
				slider *sl = reinterpret_cast<slider*>(l->second.get());
				// changes value, changing value is not possible while dragging
				if (c->listSize() == 2 && !sl->isDragging()) {
					sl->setValue((double)(c + 2)->f);
					return gl.t();
				}

				// returns value
				ret.push_back(base::cell((float)sl->getValue()));
				return ret.end();
			}
			gl.signalError("slider not found");
			return gl.nil();
		}
		gl.signalError("slider-set-value: invalid arguments, expected (id float)");
		return gl.nil();
	}

	//- bindings
	// custom add listener functors
	static void add_mouse_listener_fn(Component *com, listener *l) {
		com->addMouseListener(l, true);
	}

	static void add_slider_listener_fn(Component *com, listener *l) {
		((Slider*)com)->addListener(l);
	}

	// (bind-* (id)component (id)function (list|optional)bindings) -> bool
	template <typename T>
	base::cell_t bind_listener(base::cell_t c, base::cells_t &, void(*addFx)(Component *, listener *)) {
		if (base::lisp::validate(c, base::cell::listRange(2, 3), base::cell::typeIdentifier,
								 base::cell::typeIdentifier, base::cell::typeList)) {
			const auto &cname = c + 1;
			const auto &bname = c + 2;
			auto e = components.find(cname->s);
			if (e != components.end()) {
				Component *com = e->second.get();
				listeners.push_back(std::make_unique<T>(gl, bname->s, com));
				addFx(com, listeners.back().get());

				// iterate event properties
				if (c->listSize() == 3)
					base::lisp::mapc(c + 3, [com, this](base::cell_t c) {
							if (c->s == "selected-row")
								listeners.back()->args.push_back([com](){ return base::strs("\"", reinterpret_cast<playlist*>(com)->getSelectedRowString(), "\""); });
							if (c->s == "selected-row-index")
								listeners.back()->args.push_back([com](){ return base::toStr(reinterpret_cast<playlist*>(com)->getSelectedRowIndex()); });
							else if (c->s == "selected-row-id")
								listeners.back()->args.push_back([com](){ return base::strs("\"", reinterpret_cast<playlist*>(com)->getSelectedRowId(), "\""); });
							else if (c->s == "slider-value")
								listeners.back()->args.push_back([com](){ return base::toStr(reinterpret_cast<Slider*>(com)->getValue()); });
							else if (c->s == "component-name")
								listeners.back()->args.push_back([com](){ return base::strs("'", com->getComponentID().toStdString()); });
						});
				return gl.t();
			}
			return gl.nil();
		}
		gl.signalError("bind-*: invalid arguments, expected (id id)");
		return gl.nil();
	}

	// custom listener removers
	static void remove_mouse_listener_fx(Component *com, listener *l) {
		com->removeMouseListener(l);
	}

	static void remove_slider_listener_fx(Component *com, listener *l) {
		((Slider*)com)->removeListener(l);
	}

	// (unbind-* (id)component (id)function)
	base::cell_t unbind_listener(base::cell_t c, base::cells_t &, void(*removeFx)(Component *, listener *)) {
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
					removeFx(com, (*l).get());
					listeners.erase(l);
					return gl.t();
				}
				return gl.nil();
			}
			return gl.nil();
		}
		gl.signalError("unbind-*: invalid arguments, expected (id id)");
		return gl.nil();
	}

	//- timers
	// (create-timer (id)name (id)function) -> nil/t | name
	base::cell_t create_timer(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeIdentifier)) {
			const auto &id = c + 1;
			const auto &fx = c + 2;
			if (timers.find(id->s) == timers.end()) {
				timers[id->s] = std::make_unique<timerListener>(gl, fx->s);
				return id;
			}
			gl.signalError(base::strs("timer named: ", id->s, " already exists"));
			return gl.nil();
		}
		gl.signalError("create-timer: invalid arguments, expected (id id)");
		return gl.nil();
	}

	// (remove-timer (id)name) -> nil/t
	base::cell_t remove_timer(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &id = c + 1;
			return 1 == timers.erase(id->s) ? gl.t() : gl.nil();
		}
		gl.signalError("remove-timer: invalid arguments, expected (id)");
		return gl.nil();
	}

	// (start-timer (id)name (int)milliseconds) -> nil/t
	base::cell_t start_timer(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeInt)) {
			const auto &id = c + 1;
			const auto &freq = c + 2;
			auto e = timers.find(id->s);
			if (e != timers.end()) {
				e->second->startTimer(freq->i);
				return gl.t();
			}
			return gl.nil();
		}
		gl.signalError("start-timer: invalid arguments, expected (id int)");
		return gl.nil();
	}

	// (stop-timer (id)name) -> nil/t
	base::cell_t stop_timer(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &id = c + 1;
			auto e = timers.find(id->s);
			if (e != timers.end()) {
				e->second->stopTimer();
				return gl.t();
			}
			return gl.nil();
		}
		gl.signalError("stop-timer: invalid arguments, expected (id int)");
		return gl.nil();
	}

	//- message boxes
	// (message-box (string)caption (string)text) -> nil/t
	base::cell_t message_box(base::cell_t c, base::cells_t &) {
		if (base::lisp::validate(c, base::cell::list(2), base::cell::typeString,
								 base::cell::typeString)) {
			const auto &caption = c + 1;
			const auto &text = c + 2;
			AlertWindow::showMessageBox(AlertWindow::InfoIcon, caption->s, text->s, "ok");
			return gl.t();
		}
		gl.signalError("message-box: invalid arguments, expected (string string)");
		return gl.nil();
	}

	// (input-box (string)caption (string)text (string)input-value) -> nil | string
	base::cell_t input_box(base::cell_t c, base::cells_t &ret) {
		if (base::lisp::validate(c, base::cell::list(3), base::cell::typeString,
								 base::cell::typeString, base::cell::typeString)) {
			const auto &caption = c + 1;
			const auto &text = c + 2;
			const auto &input = c + 3;
            AlertWindow w(caption->s, text->s, AlertWindow::QuestionIcon);
            w.addTextEditor("text", input->s, "text field:");
            w.addButton("OK", 0, KeyPress(KeyPress::returnKey, 0, 0));
            if (w.runModalLoop() == 0) { // wait for OK
				ret.push_back(base::cell(w.getTextEditorContents("text").toStdString()));
				return ret.end();
            }
			return gl.nil();
		}
		gl.signalError("input-box: invalid arguments, expected (id int)");
		return gl.nil();
	}
};
