#include "includes.hpp"
#include "interface.hpp"
#include "customTags.hpp"
#include "settings.hpp"

namespace ProjectInfo {
const char* const projectName = "Kiwano";
const char* const versionString = "0.1";
const int versionNumber = 0x10000;
}

class hotkeyProcessing : public Timer {
public:
	void timerCallback() override {
		system::hotkey::process();
	}
};

class MainWindow : public DocumentWindow {
	user_interface itf;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow);
    LookAndFeel_V1 lookAndFeelV1;
    LookAndFeel_V2 lookAndFeelV2;
    LookAndFeel_V3 lookAndFeelV3;
	base::rng<> r;
	hotkeyProcessing hotkeyProcess;
	std::map<int, base::string> keyMap;

public:
	base::lisp gl;
	base::string onExit;

	MainWindow(String name) : DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons),
							  itf(gl) {
		//setUsingNativeTitleBar(true); // TODO: fix blink on startup
		setContentOwned(&itf, false);
		setSize(800, 600);
		setTopLeftPosition(200, 200);
		setResizable(true, true);
		setVisible(true);
		LookAndFeel::setDefaultLookAndFeel(&lookAndFeelV3);

		// initialize playback
		playback::init(gl);

		// initialize timer
		base::timer::init();

		// logger
		base::log::init(base::fs::getUserDirectory() + "/.kiwano/log.txt");

		// hotkeys
		system::hotkey::init();

		// initialize GLISP
		using namespace std::placeholders;
		gl.init();

		// GUI
		gl.addProcedure("create-interpreter", std::bind(&user_interface::create_interpreter, &itf, _1, _2));
		gl.addProcedure("audio-settings", std::bind(&user_interface::audio_settings, &itf, _1, _2));
		gl.addProcedure("create-text-button", std::bind(&user_interface::create_text_button, &itf, _1, _2));
		gl.addProcedure("set-main-component", std::bind(&user_interface::set_main_component, &itf, _1, _2));
		gl.addProcedure("has-component", std::bind(&user_interface::has_component, &itf, _1, _2));
		gl.addProcedure("get-components", std::bind(&user_interface::get_components, &itf, _1, _2));
		gl.addProcedure("get-child-components", std::bind(&user_interface::get_child_components, &itf, _1, _2));
		gl.addProcedure("repaint-component", std::bind(&user_interface::repaint_component, &itf, _1, _2));
		gl.addProcedure("component-enabled", std::bind(&user_interface::component_enabled, &itf, _1, _2));
		gl.addProcedure("refresh-interface", std::bind(&user_interface::refresh_interface, &itf, _1, _2));
		gl.addProcedure("unique-id", std::bind(&user_interface::unique_id, &itf, _1, _2));
		gl.addProcedure("component-centre", std::bind(&user_interface::component_centre, &itf, _1, _2));

		// GUI event binding
		gl.addProcedure("bind-mouse-click", std::bind(&user_interface::bind_listener<mouseUpListener>, &itf, _1, _2, &user_interface::add_mouse_listener_fn));
		gl.addProcedure("bind-mouse-up", std::bind(&user_interface::bind_listener<mouseUpListener>, &itf, _1, _2, &user_interface::add_mouse_listener_fn));
		gl.addProcedure("bind-mouse-double-click", std::bind(&user_interface::bind_listener<mouseDoubleClickListener>, &itf, _1, _2, &user_interface::add_mouse_listener_fn));
		gl.addProcedure("bind-mouse-down", std::bind(&user_interface::bind_listener<mouseDownListener>, &itf, _1, _2, &user_interface::add_mouse_listener_fn));
		gl.addProcedure("unbind-mouse", std::bind(&user_interface::unbind_listener, &itf, _1, _2, &user_interface::remove_mouse_listener_fx));

		// layouts
		gl.addProcedure("create-layout", std::bind(&user_interface::create_layout, &itf, _1, _2));
		gl.addProcedure("layout-add-component", std::bind(&user_interface::layout_add_component, &itf, _1, _2));
		gl.addProcedure("layout-remove-component", std::bind(&user_interface::layout_remove_component, &itf, _1, _2));
		gl.addProcedure("layout-add-splitter", std::bind(&user_interface::layout_add_splitter, &itf, _1, _2));
		gl.addProcedure("layout-get-splitter-count", std::bind(&user_interface::layout_get_splitters_count, &itf, _1, _2));
		gl.addProcedure("layout-remove-splitter", std::bind(&user_interface::layout_remove_splitter, &itf, _1, _2));

		// playlist
		gl.addProcedure("create-playlist", std::bind(&user_interface::create_playlist, &itf, _1, _2));
		gl.addProcedure("create-filtered-playlist", std::bind(&user_interface::create_filtered_playlist, &itf, _1, _2));
		gl.addProcedure("playlist-get-selected", std::bind(&user_interface::playlist_get_selected, &itf, _1, _2));
		gl.addProcedure("playlist-select", std::bind(&user_interface::playlist_select, &itf, _1, _2));
		gl.addProcedure("playlist-load", std::bind(&user_interface::playlist_load, &itf, _1, _2));
		gl.addProcedure("playlist-save", std::bind(&user_interface::playlist_save, &itf, _1, _2));
		gl.addProcedure("playlist-items-count", std::bind(&user_interface::playlist_items_count, &itf, _1, _2));
		gl.addProcedure("playlist-get", std::bind(&user_interface::playlist_get, &itf, _1, _2));
		gl.addProcedure("playlist-add-column", std::bind(&user_interface::playlist_add_column, &itf, _1, _2));
		gl.addProcedure("playlist-enable-filter", std::bind(&user_interface::playlist_enable_filter, &itf, _1, _2));
		gl.addProcedure("playlist-filter-next", std::bind(&user_interface::playlist_filter_next, &itf, _1, _2));
		gl.addProcedure("playlist-disable-filter", std::bind(&user_interface::playlist_disable_filter, &itf, _1, _2));
		gl.addProcedure("playlist-filter-enabled", std::bind(&user_interface::playlist_filter_enabled, &itf, _1, _2));

		// custom tags api
		gl.addProcedure("ctags-get", std::bind(&customTags::ctags_get, std::ref(gl), _1, _2));
		gl.addProcedure("ctags-remove", std::bind(&customTags::ctags_remove, std::ref(gl), _1, _2));
		gl.addProcedure("ctags-set", std::bind(&customTags::ctags_set, std::ref(gl), _1, _2));
		gl.addProcedure("ctags-save", std::bind(&customTags::ctags_store, std::ref(gl), _1, _2));
		gl.addProcedure("ctags-load", std::bind(&customTags::ctags_load, std::ref(gl), _1, _2));

		// settings
		gl.addProcedure("settings-load", std::bind(&settings_load, std::ref(gl), _1, _2));
		gl.addProcedure("settings-save", std::bind(&settings_save, std::ref(gl), _1, _2));
		gl.addProcedure("settings-get", std::bind(&settings_get, std::ref(gl), _1, _2));
		gl.addProcedure("settings-set", std::bind(&settings_set, std::ref(gl), _1, _2));

		// slider
		gl.addProcedure("create-slider", std::bind(&user_interface::create_slider, &itf, _1, _2));
		gl.addProcedure("slider-range", std::bind(&user_interface::slider_range, &itf, _1, _2));
		gl.addProcedure("slider-value", std::bind(&user_interface::slider_value, &itf, _1, _2));
		gl.addProcedure("bind-slider-changed", std::bind(&user_interface::bind_listener<sliderValueChangedListener>, &itf, _1, _2, &user_interface::add_slider_listener_fn));
		gl.addProcedure("bind-slider-drag-begin", std::bind(&user_interface::bind_listener<sliderDragBeginListener>, &itf, _1, _2, &user_interface::add_slider_listener_fn));
		gl.addProcedure("bind-slider-drag-end", std::bind(&user_interface::bind_listener<sliderDragEndedListener>, &itf, _1, _2, &user_interface::add_slider_listener_fn));
		gl.addProcedure("unbind-slider", std::bind(&user_interface::unbind_listener, &itf, _1, _2, &user_interface::remove_slider_listener_fx));

		// tabs
		gl.addProcedure("create-tabs", std::bind(&user_interface::create_tabs, &itf, _1, _2));
		gl.addProcedure("tabs-add-component", std::bind(&user_interface::tabs_add_component, &itf, _1, _2));
		gl.addProcedure("tabs-count", std::bind(&user_interface::tabs_count, &itf, _1, _2));
		gl.addProcedure("tabs-get-components", std::bind(&user_interface::tabs_get_components, &itf, _1, _2));
		gl.addProcedure("tabs-remove", std::bind(&user_interface::tabs_remove, &itf, _1, _2));
		gl.addProcedure("tabs-index", std::bind(&user_interface::tabs_index, &itf, _1, _2));

		// window
		gl.addProcedure("create-window", std::bind(&user_interface::create_window, &itf, _1, _2));
		gl.addProcedure("window-set-main-component", std::bind(&user_interface::window_set_main_component, &itf, _1, _2));

		// timers
		gl.addProcedure("create-timer", std::bind(&user_interface::create_timer, &itf, _1, _2));
		gl.addProcedure("remove-timer", std::bind(&user_interface::remove_timer, &itf, _1, _2));
		gl.addProcedure("start-timer", std::bind(&user_interface::start_timer, &itf, _1, _2));
		gl.addProcedure("stop-timer", std::bind(&user_interface::stop_timer, &itf, _1, _2));

		// message boxes
		gl.addProcedure("message-box", std::bind(&user_interface::message_box, &itf, _1, _2));
		gl.addProcedure("input-box", std::bind(&user_interface::input_box, &itf, _1, _2));

		// playback API
		gl.addProcedure("playback-set-file", std::bind(&playback::set_file, std::ref(gl), _1, _2));
		gl.addProcedure("playback-unload-file", std::bind(&playback::unload_file, std::ref(gl), _1, _2));
		gl.addProcedure("playback-start", std::bind(&playback::start, std::ref(gl), _1, _2));
		gl.addProcedure("playback-stop", std::bind(&playback::stop, std::ref(gl), _1, _2));
		gl.addProcedure("playback-seek", std::bind(&playback::seek, std::ref(gl), _1, _2));
		gl.addProcedure("playback-length", std::bind(&playback::length, std::ref(gl), _1, _2));
		gl.addProcedure("playback-get-pos", std::bind(&playback::get_pos, std::ref(gl), _1, _2));
		gl.addProcedure("playback-is-playing", std::bind(&playback::is_playing, std::ref(gl), _1, _2));
		gl.addProcedure("playback-finished", std::bind(&playback::finished_playing, std::ref(gl), _1, _2));
		gl.addProcedure("playback-gain", std::bind(&playback::gain, std::ref(gl), _1, _2));
		gl.addProcedure("bind-playback", std::bind(&playback::bind_playback, std::ref(gl), _1, _2));
		gl.addProcedure("unbind-playback", std::bind(&playback::unbind_playback, std::ref(gl), _1, _2));

		// other utils
		gl.addProcedure("rand", std::bind(&MainWindow::random, this, _1, _2));
		gl.addProcedure("current-time", std::bind(&MainWindow::current_time, this, _1, _2));
		gl.addProcedure("time-format", std::bind(&MainWindow::time_format, this, _1, _2));
		gl.addProcedure("bind-hotkey", std::bind(&MainWindow::bind_hotkey, this, _1, _2));
		gl.addProcedure("unbind-hotkey", std::bind(&MainWindow::unbind_hotkey, this, _1, _2));
		gl.addProcedure("bind-key", std::bind(&MainWindow::bind_key, this, _1, _2));
		gl.addProcedure("unbind-key", std::bind(&MainWindow::unbind_key, this, _1, _2));

		// exit handler
		gl.addProcedure("bind-exit", std::bind(&MainWindow::bind_exit, this, _1, _2));

		// prepare settings folder
		base::string appPath = base::fs::getUserDirectory() + "/.kiwano";
		base::fs::createFolderTree(appPath);
		base::fs::open(appPath);

		// load ELISP config file
		gl.eval(base::toStr(base::fs::load("init.lisp")));
	}

	//- LISP API
	// (bind-exit (id)function) -> nil/t
	base::cell_t bind_exit(base::cell_t c, base::cells_t &) {
		return fxValidateSkeleton(gl, "bind-exit", c, [this, c]() -> auto {
				const auto &callback = c + 1;
				onExit = callback->s;
				return gl.t();
			}, base::cell::list(1), base::cell::typeIdentifier);
	}

	// (rand (int)max) -> nil/t
	base::cell_t random(base::cell_t c, base::cells_t &ret) {
		return fxValidateSkeleton(gl, "rand", c, [&ret, c, this]() -> auto {
				const auto &max = c + 1;
				ret.push_back(r.integer(max->i));
				return ret.end();
			}, base::cell::list(1), base::cell::typeInt);
	}

	// (current-time) -> int (unix)
	base::cell_t current_time(base::cell_t, base::cells_t &ret) {
		ret.push_back((int)time(NULL));
		return ret.end();
	}

	// (time-format (int)time (string)format) -> string/nil
	base::cell_t time_format(base::cell_t c, base::cells_t &ret) {
		return fxValidateSkeleton(gl, "time-format", c, [c, &ret]() -> auto {
				const auto &unixTime = c + 1;
				const auto &format = c + 2;
				time_t rawTime = (time_t)unixTime->i;
				std::stringstream ss;
				ss << std::put_time(localtime(&rawTime), format->s.c_str());
				ret.push_back(ss.str());
				return ret.end();
			}, base::cell::list(2), base::cell::typeInt, base::cell::typeString);
	}

	// (bind-hotkey (string|int)key (string|int)mod (id)bind) -> nil/(int)id
	base::cell_t bind_hotkey(base::cell_t c, base::cells_t &ret) {
		using namespace base;
		return fxValidateSkeleton(gl, "bind-hotkey", c, [this, c, &ret]() -> auto {
				const auto &key = c + 1;
				const auto &mod = c + 2;
				const auto &fx = c + 3;
				const string fxName = fx->s;

				// register hotkey in system
				int bound = system::hotkey::add(key->type == cell::typeInt ? key->i : system::getKey(key->s),
												mod->type == cell::typeInt ? mod->i : system::getModifier(mod->s),
												[this, fxName]() {
													gl.eval(strs("(", fxName, ")"));
												});

				// return bind id
				if (bound > 0) {
					hotkeyProcess.startTimer(200);
					ret.push_back({bound});
					return ret.end();
				}

				return gl.nil();
			}, cell::list(3), cell::anyOf(cell::typeString, cell::typeInt), cell::anyOf(cell::typeString, cell::typeInt), cell::typeIdentifier);
	}

	// (unbind-hotkey (int)id) -> nil/t
	base::cell_t unbind_hotkey(base::cell_t c, base::cells_t &) {
		return fxValidateSkeleton(gl, "unbind-hotkey", c, [this, c]() -> auto {
				const auto &id = c + 1;
				bool unbound = system::hotkey::remove(id->i);
				return unbound ? gl.t() : gl.nil();
			}, base::cell::list(1), base::cell::typeInt);
	}

	// (bind-key (string)key-desc (id)callback)
	base::cell_t bind_key(base::cell_t c, base::cells_t &) {
		return fxValidateSkeleton(gl, "bind-key", c, [this, c]() -> auto {
				const auto &key = c + 1;
				const auto &callback = c + 2;
				KeyPress kp = KeyPress::createFromDescription(key->s);
				if (kp.isValid()) {
					int keyCode = kp.getKeyCode();
					keyMap[keyCode] = callback->s;
					return gl.t();
				}
				return gl.nil();
			}, base::cell::list(2), base::cell::typeString, base::cell::typeIdentifier);
	}

	// (unbind-key (string)key-desc)
	base::cell_t unbind_key(base::cell_t c, base::cells_t &) {
		return fxValidateSkeleton(gl, "unbind-key", c, [this, c]() -> auto {
				const auto &key = c + 1;
				KeyPress kp = KeyPress::createFromDescription(key->s);
				if (kp.isValid()) {
					int keyCode = kp.getKeyCode();
					auto e = keyMap.find(keyCode);
					if (e != keyMap.end()) {
						keyMap.erase(e);
						return gl.t();
					}
				}
				return gl.nil();
			}, base::cell::list(1), base::cell::typeString);
	}

	void cleanup() {
		system::hotkey::shutdown();
		base::fs::close();
		gl.close();
		base::log::shutdown();
		playback::shutdown();
	}

	void closeButtonPressed() override {
		JUCEApplication::getInstance()->systemRequestedQuit();
	}

	bool keyPressed(const KeyPress &k) override {
		auto e = keyMap.find(k.getKeyCode());
		if (e != keyMap.end()) {
			gl.eval(base::strs("(", e->second, ")"));
			return true;
		}
		return false;
	}
};

class KiwanoApplication : public JUCEApplication {
    ScopedPointer<MainWindow> mainWindow;

public:
    KiwanoApplication() {}

    const String getApplicationName() override { return ProjectInfo::projectName; }
    const String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const String& commandLine) override {
        mainWindow = new MainWindow(getApplicationName());
    }

    void shutdown() override {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override {
		mainWindow->gl.eval(base::strs("(", mainWindow->onExit, ")"));
		mainWindow->cleanup();
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override {
    }
};

START_JUCE_APPLICATION(KiwanoApplication);

// TODO: load default config
// TODO: lisp include
// TODO: shortcut to open interpreter
// TODO: error log/console window (only for lisp?)
// TODO: playlist: get-selected-hash
// TODO: attach granite logger
// TODO: on start, on close
// TODO: get-cpu-usage
// TODO: consider fully manual audio settings (as additional interface or replace current one)
// TODO: audio buffer size settings load/store
// TODO: CUE support
// TODO: playlist configuration
// TODO: playlist groups
// TODO: playlist image
// TODO: remove playlist-get-selected?
// TODO: copy-file with new name
// TODO: file manipulation?
// TODO: repaint-row?
// TODO: global keybinding
// TODO: bug: de/serialization of playlists when audio options or interpreters are open
// TODO: stop timer when no binds (need update granite API)
// TODO: windows positioning (center screen) (bounds-center-screen w h)
// TODO: window closing
// TODO: (de)serialize window position (see: restoreWindowStateFromString)
// TODO: timer threading issues
// TODO: "invisible" mainWindow
