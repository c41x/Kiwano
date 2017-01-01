#pragma once
#include "includes.hpp"
#include "tabs.hpp"
#include "interpreter.hpp"
#include "layout.hpp"
#include "playback.hpp"
#include "listeners.hpp"
#include "slider.hpp"
#include "window.hpp"
#include "fxTemplates.hpp"

// change listener
class audioSettingsChangeListener : public ChangeListener {
public:
    AudioDeviceManager *am;
    audioSettingsChangeListener() : am(nullptr) {}
    void changeListenerCallback(ChangeBroadcaster *source) override {
        if (am == source && am) {
            logInfo("storing audio settings...");
            if (am->createStateXml()) {
                logInfo("saving audio settings to \"audio-settings.xml\"");
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
                                        asListener(std::make_unique<audioSettingsChangeListener>()) { }
    ~user_interface() { }

    void resized() override {
        if (mainComponent)
            mainComponent->setBounds(getLocalBounds());
    }

    void cleanup() {
        // stop all timers
        for (auto &t : timers) {
            t.second->stopTimer();
        }
    }

    // curry skeleton templates, TODO: simplify this?
    template <typename... Args, typename T>
    base::cell_t fxValidate(const base::string &fxName, base::cell_t c, T fx, Args... v) {
        return fxValidateSkeleton(gl, fxName, c, fx, v...);
    }

    template <typename... Args, typename T, typename TC>
    base::cell_t fxValidateAccess(const base::string &fxName, base::cell_t c, T fx, TC &container, Args... v) {
        return fxValidateAccessSkeleton(gl, fxName, c, fx, container, v...);
    }

    template <typename... Args, typename T, typename TC>
    base::cell_t fxValidateAccessCreate(const base::string &fxName, base::cell_t c, T fx, TC &container, Args... v) {
        return fxValidateAccessCreateSkeleton(gl, fxName, c, fx, container, v...);
    }

    template <typename... Args, typename T, typename TC>
    base::cell_t fxValidateTryAccess(const base::string &fxName, base::cell_t c, T fx, TC &container, Args... v) {
        return fxValidateTryAccessSkeleton(gl, fxName, c, fx, container, v...);
    }

    template <typename... Args, typename T, typename TC>
    base::cell_t fxValidateCreate(const base::string &fxName, base::cell_t c, T fx, TC &container, Args... v) {
        return fxValidateCreateSkeleton(gl, fxName, c, fx, container, v...);
    }

    template <typename... Args, typename T, typename TC>
    base::cell_t fxValidateAccess2(const base::string &fxName, base::cell_t c, T fx, TC &container, Args... v) {
        return fxValidateAccess2Skeleton(gl, fxName, c, fx, container, v...);
    }

    //- general GUI
    // (set-main-component name) -> bool
    base::cell_t set_main_component(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("set-main-component", c, [c, this](Component *cc) -> auto {
                mainComponent = cc;
                addAndMakeVisible(mainComponent);
                mainComponent->setBounds(getLocalBounds());
                return gl.t();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    // (has-component name) -> bool
    base::cell_t has_component(base::cell_t c, base::cells_t &) {
        return fxValidateTryAccess("has-component", c, [c, this](Component *) -> auto {
                return gl.t();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    // (repaint-component name) -> bool
    base::cell_t repaint_component(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("repaint-component", c, [this](Component *c) -> auto {
                c->repaint();
                return gl.t();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    // (get-components (id)type) -> (list of id)
    base::cell_t get_components(base::cell_t c, base::cells_t &ret) {
        return fxValidate("get-components", c, [c, &ret, this]() -> auto {
                bool all = c->listSize() == 0; // if there are no arguments - get all components
                const auto &type = c + 1;
                ret.push_back(base::cell::list(0));
                auto &head = ret.back();
                for (const auto &com : components) {
                    if (all || com.second->getName() == type->s) {
                        ret.push_back(base::cell(base::cell::typeIdentifier, com.first));
                        head.i++;
                    }
                }
                return ret.end();
            }, base::cell::listRange(0, 1), base::cell::typeIdentifier);
    }

    // (get-child-components (id)type) -> (list of id)
    base::cell_t get_child_components(base::cell_t c, base::cells_t &ret) {
        return fxValidateAccess("get-child-components", c, [c, &ret, this](Component *e) -> auto {
                const auto &type = c + 2;
                ret.push_back(base::cell::list(0));
                auto &head = ret.back();

                auto addToList = [&ret, &head](Component *c) {
                    ret.push_back(base::cell(base::cell::typeIdentifier,
                                             c->getComponentID().toStdString()));
                    head.i++;
                };

                // if component is 'tabs iterate through it's items
                if (e->getName() == "tabs") {
                    tabs *t = reinterpret_cast<tabs*>(e);
                    for (int i = 0; i < t->getNumTabs(); ++i) {
                        auto tt = t->getTabContentComponent(i);
                        if (tt->getName() == type->s) {
                            addToList(tt);
                        }
                    }
                }
                else {
                    // default search in JUCE hierarchy
                    for (int i = 0; i < e->getNumChildComponents(); ++i) {
                        Component *com = e->getChildComponent(i);
                        if (com->getName() == type->s) {
                            addToList(com);
                        }
                    }
                }
                return ret.end();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeIdentifier);
    }

    // (component-enabled name (optional|nil/t)) -> bool
    base::cell_t component_enabled(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateAccess("component-enabled", c, [c, this](Component *e) -> auto {
                if (c->listSize() == 1) {
                    // getter
                    if (e->isEnabled())
                        return gl.t();
                    return gl.nil();
                }

                // setter
                const auto &doEnable = c + 2;
                e->setEnabled(!doEnable->isNil());
                return doEnable;
            }, components, cell::listRange(1, 2), cell::typeIdentifier, cell::typeIdentifier);
    }

    // TODO: (component-centre (id)name (optional|int)width (optional|int)height) -> bool
    base::cell_t component_centre(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateAccess("component-centre", c, [c, this](Component *com) -> auto {
                int w, h;
                if (c->listSize() == 3) {
                    w = (c + 2)->i;
                    h = (c + 2)->i;
                }
                else {
                    w = com->getWidth();
                    h = com->getHeight();
                }
                com->centreWithSize(w, h);
                return gl.t();
            }, components, cell::any(cell::list(1), cell::list(3)), cell::typeIdentifier, cell::typeInt, cell::typeInt);
    }

    // (refresh-interface)
    base::cell_t refresh_interface(base::cell_t, base::cells_t &) {
        // signal to all components resized event (to refresh UI)
        for (auto &c : components)
            c.second->resized();
        return gl.t();
    }

    // (unique-id (string)prefix) -> id / nil
    base::cell_t unique_id(base::cell_t c, base::cells_t &ret) {
        return fxValidate("unique-id", c, [c, &ret, this]() -> auto {
                const auto &prefix = c + 1;
                base::string gid;
                for (int32 n = 0; n < 99999; ++n) {
                    gid = base::strs(prefix->s, "_", n);
                    if (components.count(gid) == 0) {
                        ret.push_back({base::cell::typeIdentifier, gid});
                        return ret.end();
                    }
                }
                return gl.nil();
            }, base::cell::list(1), base::cell::typeString);
    }

    //- playlist
    // (create-playlist name) -> nil/id
    base::cell_t create_playlist(base::cell_t c, base::cells_t &) {
        return fxValidateCreate("create-playlist", c, [c, this]() -> auto {
                const auto &name = c + 1;
                auto &p = components[name->s] = std::make_unique<playlist>();
                p->setName("playlist");
                p->setComponentID(name->s);
                return name;
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    // (create-filtered-playlist (id)base-playlist (id)new-playlist (string)filter) -> name/nil
    base::cell_t create_filtered_playlist(base::cell_t c, base::cells_t &) {
        return fxValidateAccessCreate("create-filtered-playlist", c, [this, c](Component *com) -> auto {
                const auto &name = c + 2;
                const auto &filter = c + 3;
                auto &p = components[name->s] = std::make_unique<playlist>(*((playlist*)com), filter->s);
                p->setName("playlist");
                p->setComponentID(name->s);
                return name;
            }, components, base::cell::list(3), base::cell::typeIdentifier, base::cell::typeIdentifier, base::cell::typeString);
    }

    base::cell_t playlist_select(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("playlist-select", c, [c, this](Component *e) -> auto {
                const auto &row = c + 2;
                auto p = reinterpret_cast<playlist*>(e);
                p->selectRow(row->i);
                return gl.t();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeInt);
    }

    base::cell_t playlist_items_count(base::cell_t c, base::cells_t &ret) {
        return fxValidateAccess("playlist-items-count", c, [&ret](Component *e) -> auto {
                auto p = reinterpret_cast<playlist*>(e);
                ret.push_back(base::cell(base::cell::typeInt, p->getItemsCount()));
                return ret.end();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    base::cell_t playlist_get(base::cell_t c, base::cells_t &ret) {
        using namespace base;
        return fxValidateAccess("playlist-get", c, [&ret, this, c](Component *e) -> auto {
                const auto &index = c + 2;
                const auto &query = c + 3;
                auto p = reinterpret_cast<playlist*>(e);
                if (query->s == "id")
                    ret.push_back(cell(cell::typeString, p->getRowId(index->i)));
                else if (query->s == "path")
                    ret.push_back(cell(cell::typeString, p->getRowPath(index->i)));
                else if (query->s == "path-raw")
                    ret.push_back(cell(cell::typeString, p->getRowPathRaw(index->i)));
                else return gl.nil();
                return ret.end();
            }, components, cell::list(3), cell::typeIdentifier, cell::typeInt, cell::typeIdentifier);
    }

    base::cell_t playlist_load(base::cell_t c, base::cells_t &ret) {
        return fxValidateAccess("playlist-load", c, [c, this](Component *e) -> auto {
                const auto &path = c + 2;
                auto p = reinterpret_cast<playlist*>(e);
                if (p->load(path->s))
                    return gl.t();
                return gl.nil();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeString);
    }

    base::cell_t playlist_save(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("playlist-save", c, [c, this](Component *e) -> auto {
                const auto &path = c + 2;
                auto p = reinterpret_cast<playlist*>(e);
                if (p->store(path->s))
                    return gl.t();
                return gl.nil();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeString);
    }

    // (playlist-add-column (id)playlist (string)caption (string)content (int)width (int)minWidth (int)maxWidth)
    base::cell_t playlist_add_column(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateAccess("playlist-add-column", c, [c, this](Component *e) -> auto {
                const auto &caption = c + 2;
                const auto &content = c + 3;
                const auto &width = c + 4;
                const auto &minWidth = c + 5;
                const auto &maxWidth = c + 6;
                auto p = reinterpret_cast<playlist*>(e);
                p->addColumn(caption->s, content->s, width->i, minWidth->i, maxWidth->i);
                return gl.t();
            }, components, cell::list(6), cell::typeIdentifier, cell::typeString, cell::typeString,
            cell::typeInt, cell::typeInt, cell::typeInt);
    }

    base::cell_t playlist_enable_filter(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("playlist-enable-filter", c, [c, this](Component *e) -> auto {
                const auto &query = c + 2;
                auto p = reinterpret_cast<playlist*>(e);
                p->filterEnable(query->s);
                return gl.t();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeString);
    }

    base::cell_t playlist_filter_next(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("playlist-filter-next", c, [c, this](Component *e) -> auto {
                auto p = reinterpret_cast<playlist*>(e);
                p->filterSelectNext();
                return gl.t();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    base::cell_t playlist_disable_filter(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("playlist-disable-filter", c, [this](Component *e) -> auto {
                auto p = reinterpret_cast<playlist*>(e);
                p->filterDisable();
                return gl.t();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    base::cell_t playlist_filter_enabled(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("playlist-filter-enabled", c, [this](Component *e) -> auto {
                auto p = reinterpret_cast<playlist*>(e);
                if (p->filterEnabled())
                    return gl.t();
                return gl.nil();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    //- text button
    // (create-text-button name (string)caption (string)tooltip) -> nil/id
    base::cell_t create_text_button(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateCreate("create-text-button", c, [c, this]() -> auto {
                const auto &name = c + 1;
                const auto &label = c + 2;
                const auto &tip = c + 3;
                auto &p = components[name->s] = std::make_unique<TextButton>(label->s, tip->s);
                p->setName("text-button");
                p->setComponentID(name->s);
                return name;
            }, components, cell::list(3), cell::typeIdentifier, cell::typeString, cell::typeString);
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
        return fxValidateCreate("create-interpreter", c, [c, this]() -> auto {
                const auto &name = c + 1;
                auto &p = components[name->s] = std::make_unique<interpreter>(gl);
                p->setName("interpreter");
                p->setComponentID(name->s);
                return name;
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    //- windows
    // (create-window (id)name (string)caption (vector)bounds (vector)background-color)
    base::cell_t create_window(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateCreate("create-window", c, [c, this]() -> auto {
                const auto &name = c + 1;
                const auto &caption = c + 2;
                const auto &bounds = c + 3;
                const auto &color = c + 4;
                auto &w = components[name->s] =
                    std::make_unique<window>(gl, caption->s,
                                             Colour::fromFloatRGBA(color->v4[0],
                                                                   color->v4[1],
                                                                   color->v4[2],
                                                                   color->v4[3]),
                                             window::allButtons);

                window *dw = (window*)w.get();
                juce::Rectangle<int> area((int)bounds->v4[0],
                                          (int)bounds->v4[1],
                                          (int)bounds->v4[2],
                                          (int)bounds->v4[3]);
                dw->setBounds(area);
                dw->setResizable(true, true);
                dw->setUsingNativeTitleBar(false);
                dw->setVisible(true);

                return name;
            }, components, cell::list(4), cell::typeIdentifier, cell::typeString, cell::typeVector, cell::typeVector);
    }

    // (window-set-main-component (id)window-name (id)component-id) -> t/nil
    base::cell_t window_set_main_component(base::cell_t c, base::cells_t &) {
        return fxValidateAccess2("window-set-main-component", c, [this](Component *wnd, Component *com) -> auto {
                auto w = reinterpret_cast<window*>(wnd);
                com->setBounds(w->getLocalBounds());
                w->setContentOwned(com, true);
                return gl.t();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeIdentifier);
    }

    // (window-state (id)window-name (string|optional)state) -> string/nil | t/nil
    base::cell_t window_state(base::cell_t c, base::cells_t &ret) {
        return fxValidateAccess("window-state", c, [this, &ret, c](Component *wnd) -> auto {
                auto w = reinterpret_cast<window*>(wnd);

                // getter
                if (c->listSize() == 1) {
                    ret.push_back(w->getWindowStateAsString().toStdString());
                    return ret.end();
                }

                // setter
                const auto state = c + 2;
                if (w->restoreWindowStateFromString(state->s))
                    return gl.t();
                return gl.nil();
            }, components, base::cell::listRange(1, 2), base::cell::typeIdentifier, base::cell::typeString);
    }

    //- layout
    // (create-layout name (bool)horizontal) -> bool/id
    base::cell_t create_layout(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateCreate("create-layout", c, [c, this]() -> auto {
                const auto &name = c + 1;
                const auto &horizontal = c + 2;
                auto &p = components[name->s] = std::make_unique<layout>(!horizontal->isNil());
                p->setName("layout");
                p->setComponentID(name->s);
                return name;
            }, components, cell::list(2), cell::typeIdentifier, cell::typeIdentifier);
    }

    // (layout-add-component layout-id component-id (float)min (float)max (float)preffered) -> bool
    base::cell_t layout_add_component(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateAccess2("layout-add-component", c, [c, this](Component *lay, Component *com) -> auto {
                const auto &minimum = c + 3;
                const auto &maximum = c + 4;
                const auto &preferred = c + 5;
                layout *l = reinterpret_cast<layout*>(lay);
                l->addComponent(com, (double)minimum->f, (double)maximum->f, (double)preferred->f);
                return gl.t();
            }, components, cell::list(5), cell::typeIdentifier, cell::typeIdentifier,cell::typeFloat, cell::typeFloat, cell::typeFloat);
    }

    // (layout-remove-component layout-id component-id) -> bool
    base::cell_t layout_remove_component(base::cell_t c, base::cells_t &) {
        return fxValidateAccess2("layout-remove-component", c, [this](Component *lay, Component *com) -> auto {
                layout *l = reinterpret_cast<layout*>(lay);
                l->removeComponent(com);
                return gl.t();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeIdentifier);
    }

    // (layout-remove-splitter layout-id (int)splitter-index)
    base::cell_t layout_remove_splitter(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("layout-remove-splitter", c, [c, this](Component *com) -> auto {
                const auto sIndex = c + 2;
                layout *lay = reinterpret_cast<layout*>(com);
                lay->removeSplitter(sIndex->i);
                return gl.t();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeInt);
    }

    // (layout-add-splitter layout-id)
    base::cell_t layout_add_splitter(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("layout-add-splitter", c, [this](Component *com) -> auto {
                layout *lay = reinterpret_cast<layout*>(com);
                lay->addSplitter();
                return gl.t();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    // (layout-get-splitters-count layout-id)
    base::cell_t layout_get_splitters_count(base::cell_t c, base::cells_t &ret) {
        return fxValidateAccess("layout-get-splitters-count", c, [&ret](Component *com) -> auto {
                layout *lay = reinterpret_cast<layout*>(com);
                ret.push_back(base::cell(lay->getSplittersCount()));
                return ret.end();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    //- tabs
    // (create-tabs name 'orientation{top, bottom, left, right}) -> bool/id
    base::cell_t create_tabs(base::cell_t c, base::cells_t &) {
        return fxValidateCreate("create-tabs", c, [c,this]() -> auto {
                const auto &name = c + 1;
                const auto &orientation = c + 2;
                TabbedButtonBar::Orientation o = TabbedButtonBar::TabsAtTop;
                if (orientation->s == "bottom")
                    o = TabbedButtonBar::TabsAtBottom;
                else if (orientation->s == "left")
                    o = TabbedButtonBar::TabsAtLeft;
                else if (orientation->s == "right")
                    o = TabbedButtonBar::TabsAtRight;
                auto &p = components[name->s] = std::make_unique<tabs>(o);
                p->setName("tabs");
                p->setComponentID(name->s);
                return name;
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeIdentifier);
    }

    // (tabs-add-component tabs-name component-name "caption" |color|) -> bool
    base::cell_t tabs_add_component(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateAccess2("tabs-add-component", c, [c, this](Component *tab, Component *com) -> auto {
                const auto &caption = c + 3;
                const auto &color = c + 4;
                tabs *ptabs = reinterpret_cast<tabs*>(tab);
                ptabs->addTab(caption->s, Colour::fromFloatRGBA(color->v4[0],
                                                                color->v4[1],
                                                                color->v4[2],
                                                                color->v4[3]), com, false);
                return gl.t();
            }, components, cell::list(4), cell::typeIdentifier, cell::typeIdentifier, cell::typeString, cell::typeVector);
    }

    // (tabs-index (id)tabs (int|string|optional)index)
    base::cell_t tabs_index(base::cell_t c, base::cells_t &ret) {
        using namespace base;
        return fxValidateAccess("tabs-index", c, [c, this, &ret](Component *e) -> auto {
                tabs *t = reinterpret_cast<tabs*>(e);

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
            }, components, cell::listRange(1, 2), cell::typeIdentifier, cell::anyOf(cell::typeInt, cell::typeString));
    }

    // (tabs-count (id)tabs)
    base::cell_t tabs_count(base::cell_t c, base::cells_t &ret) {
        return fxValidateAccess("tabs-count", c, [&ret, this](Component *e) -> auto {
                tabs *t = reinterpret_cast<tabs*>(e);
                ret.push_back(base::cell((int32)t->getNumTabs()));
                return ret.end();
            }, components, base::cell::list(1), base::cell::typeIdentifier);
    }

    // (tabs-get-components (id)tabs (id)type)
    base::cell_t tabs_get_components(base::cell_t c, base::cells_t &ret) {
        using namespace base;
        return fxValidateAccess("tabs-get-components", c, [c, &ret, this](Component *e) -> auto {
                const auto &type = c + 2;
                tabs *t = reinterpret_cast<tabs*>(e);
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
            }, components, cell::list(2), cell::typeIdentifier, cell::typeIdentifier);
    }

    // (tabs-remove (id)tabs (int|string)index)
    base::cell_t tabs_remove(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateAccess("tabs-remove", c, [c, this](Component *e) -> auto {
                tabs *t = reinterpret_cast<tabs*>(e);
                const auto &index = c + 2;
                if (index->type == cell::typeInt)
                    t->removeTabByIndex(index->i);
                else t->removeTabByName(index->s);
                return gl.t();
            }, components, cell::list(2), cell::typeIdentifier, cell::anyOf(cell::typeInt, cell::typeString));
    }

    //- slider
    // (create-slider name 'style 'edit-box) -> nil/id
    base::cell_t create_slider(base::cell_t c, base::cells_t &) {
        using namespace base;
        return fxValidateCreate("create-slider", c, [c, this]() -> auto {
                const auto &name = c + 1;
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
                auto &p = components[name->s] = std::make_unique<slider>(style, editBox);
                p->setName("slider");
                p->setComponentID(name->s);
                return name;
            }, components, cell::listRange(1, 3), cell::typeIdentifier, cell::typeIdentifier, cell::typeIdentifier);
    }

    // (slider-range (id)slider-id (float|optional)range-min (float|optional)range-max) -> nil/t | (float float)value
    base::cell_t slider_range(base::cell_t c, base::cells_t &ret) {
        using namespace base;
        return fxValidateAccess("slider-range", c, [c, &ret, this](Component *l) -> auto {
                slider *sl = reinterpret_cast<slider*>(l);
                if (c->listSize() == 3) {
                    sl->setRange((double)(c + 2)->f,
                                 (double)(c + 3)->f);
                    return gl.t();
                }

                // getter (returns list tuple with minumum and maximum values)
                ret.push_back(cell::list(2));
                ret.push_back(cell((float)sl->getMinimum()));
                ret.push_back(cell((float)sl->getMaximum()));
                return ret.end();
            }, components, cell::any(cell::list(1), cell::list(3)), cell::typeIdentifier, cell::typeFloat, cell::typeFloat);
    }

    // (slider-value (id)slider-id (float|optional)value) -> nil/t | (float)value
    base::cell_t slider_value(base::cell_t c, base::cells_t &ret) {
        using namespace base;
        return fxValidateAccess("slider-set-value", c, [c, &ret, this](Component *l) -> auto {
                slider *sl = reinterpret_cast<slider*>(l);
                // changes value, changing value is not possible while dragging
                if (c->listSize() == 2 && !sl->isDragging()) {
                    sl->setValue((double)(c + 2)->f);
                    return gl.t();
                }

                // returns value
                ret.push_back(base::cell((float)sl->getValue()));
                return ret.end();
            }, components, cell::listRange(1, 2), cell::typeIdentifier, cell::typeFloat);
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
        return fxValidateAccess("bind-*", c, [c, addFx, this](Component *com) -> auto {
                const auto &bname = c + 2;
                listeners.push_back(std::make_unique<T>(gl, bname->s, com));
                addFx(com, listeners.back().get());

                // iterate event properties
                if (c->listSize() == 3)
                    base::lisp::mapc(c + 3, [com, this](base::cell_t c) {
                            if (c->s == "selected-row")
                                listeners.back()->args.push_back([com](){ return base::strs("\"", reinterpret_cast<playlist*>(com)->getSelectedRowPath(), "\""); });
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
            }, components, base::cell::listRange(2, 3), base::cell::typeIdentifier, base::cell::typeIdentifier, base::cell::typeList);
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
        return fxValidateAccess("unbind-*", c, [c, removeFx, this](Component *com) -> auto {
                const auto &bname = c + 2;
                auto l = std::find_if(listeners.begin(), listeners.end(), [&com, &bname](const auto &a) {
                        return a->c == com && a->functionId == bname->s;
                    });
                if (l != listeners.end()) {
                    removeFx(com, (*l).get());
                    listeners.erase(l);
                    return gl.t();
                }
                return gl.nil();
            }, components, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeIdentifier);
    }

    //- timers
    // (create-timer (id)name (id)function) -> nil/t | name
    base::cell_t create_timer(base::cell_t c, base::cells_t &) {
        return fxValidateCreate("create-timer", c, [c, this]() -> auto {
                const auto &id = c + 1;
                const auto &fx = c + 2;
                timers[id->s] = std::make_unique<timerListener>(gl, fx->s);
                return id;
            }, timers, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeIdentifier);
    }

    // (remove-timer (id)name) -> nil/t
    base::cell_t remove_timer(base::cell_t c, base::cells_t &) {
        return fxValidate("remove-timer", c, [c, this]() -> auto {
                const auto &id = c + 1;
                return 1 == timers.erase(id->s) ? gl.t() : gl.nil();
            }, base::cell::list(1), base::cell::typeIdentifier);
    }

    // (start-timer (id)name (int)milliseconds) -> nil/t
    base::cell_t start_timer(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("start-timer", c, [c, this](timerListener *t) -> auto {
                const auto &freq = c + 2;
                t->startTimer(freq->i);
                return gl.t();
            }, timers, base::cell::list(2), base::cell::typeIdentifier, base::cell::typeInt);
    }

    // (stop-timer (id)name) -> nil/t
    base::cell_t stop_timer(base::cell_t c, base::cells_t &) {
        return fxValidateAccess("stop-timer", c, [c, this](timerListener *t) -> auto {
                t->stopTimer();
                return gl.t();
            }, timers, base::cell::list(1), base::cell::typeIdentifier);
    }

    //- message boxes
    // (message-box (string)caption (string)text) -> nil/t
    base::cell_t message_box(base::cell_t c, base::cells_t &) {
        return fxValidate("message-box", c, [c, this]() -> auto {
                const auto &caption = c + 1;
                const auto &text = c + 2;
                AlertWindow::showMessageBox(AlertWindow::InfoIcon, caption->s, text->s, "ok");
                return gl.t();
            }, base::cell::list(2), base::cell::typeString, base::cell::typeString);
    }

    // (input-box (string)caption (string)text (string)input-value (id)callback) -> nil | string
    base::cell_t input_box(base::cell_t c, base::cells_t &ret) {
        using namespace base;
        return fxValidate("input-box", c, [c, &ret, this]() -> auto {
                const auto &caption = c + 1;
                const auto &text = c + 2;
                const auto &input = c + 3;
                const auto &callback = c + 4;

                auto w = new alertWindowListener(gl, callback->s, caption->s, text->s); // juce's ModalComponentManager deletes automatically this instance
                w->addTextEditor("text", input->s, "text field:");
                w->addButton("OK", 0, KeyPress(KeyPress::returnKey, 0, 0));
                w->enterModalState(true, w, false);

                return gl.nil();
            }, cell::list(4), cell::typeString, cell::typeString, cell::typeString, cell::typeIdentifier);
    }
};
