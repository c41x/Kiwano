#include "includes.hpp"
#include "interface.hpp"
#include "customTags.hpp"
#include "settings.hpp"

namespace ProjectInfo {
const char* const projectName = "Kiwano";
const char* const versionString = "0.1";
const int versionNumber = 0x10000;
}

class MainWindow : public DocumentWindow {
	user_interface itf;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow);
    LookAndFeel_V1 lookAndFeelV1;
    LookAndFeel_V2 lookAndFeelV2;
    LookAndFeel_V3 lookAndFeelV3;

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

		// logger
		base::log::init(base::fs::getUserDirectory() + "/.kiwano/log.txt");

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
		gl.addProcedure("playlist-get-selected", std::bind(&user_interface::playlist_get_selected, &itf, _1, _2));
		gl.addProcedure("playlist-load", std::bind(&user_interface::playlist_load, &itf, _1, _2));
		gl.addProcedure("playlist-save", std::bind(&user_interface::playlist_save, &itf, _1, _2));
		gl.addProcedure("playlist-items-count", std::bind(&user_interface::playlist_items_count, &itf, _1, _2));
		gl.addProcedure("playlist-get", std::bind(&user_interface::playlist_get, &itf, _1, _2));
		gl.addProcedure("playlist-add-column", std::bind(&user_interface::playlist_add_column, &itf, _1, _2));

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
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &callback = c + 1;
			onExit = callback->s;
			return gl.t();
		}
		gl.signalError("bind-exit: invalid arguments, expected (id)");
		return gl.nil();
	}

	void cleanup() {
		base::fs::close();
		gl.close();
		base::log::shutdown();
		playback::shutdown();
	}

	void closeButtonPressed() override {
		JUCEApplication::getInstance()->systemRequestedQuit();
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
// TODO: load/store named value
// TODO: playlist: get-selected-hash
// TODO: property map (store-property id value) (load-property id value)
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
// TODO: repaint-row?
// TODO: spawn window
// TODO: get*components - get all components without specifying type
