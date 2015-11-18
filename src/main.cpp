#include "includes.hpp"
#include "interface.hpp"

namespace ProjectInfo {
const char* const projectName = "Kiwano";
const char* const versionString = "0.1";
const int versionNumber = 0x10000;
}

class MainWindow : public DocumentWindow {
	user_interface itf;
	base::lisp gl;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow);

public:
	MainWindow(String name) : DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons),
							  itf(gl) {
		//setUsingNativeTitleBar(true); // TODO: fix blink on startup
		setContentOwned(&itf, false);
		setSize(800, 600);
		setTopLeftPosition(200, 200);
		setResizable(true, true);
		setVisible(true);

		// initialize playback
		playback::init();

		// initialize GLISP
		using namespace std::placeholders;
		gl.init();

		// GUI
		gl.addProcedure("create-playlist", std::bind(&user_interface::create_playlist, &itf, _1, _2));
		gl.addProcedure("create-layout", std::bind(&user_interface::create_layout, &itf, _1, _2));
		gl.addProcedure("create-interpreter", std::bind(&user_interface::create_interpreter, &itf, _1, _2));
		gl.addProcedure("create-tabs", std::bind(&user_interface::create_tabs, &itf, _1, _2));
		gl.addProcedure("create-audio-settings", std::bind(&user_interface::create_audio_settings, &itf, _1, _2));
		gl.addProcedure("create-text-button", std::bind(&user_interface::create_text_button, &itf, _1, _2));
		gl.addProcedure("layout-add-component", std::bind(&user_interface::layout_add_component, &itf, _1, _2));
		gl.addProcedure("layout-remove-component", std::bind(&user_interface::layout_remove_component, &itf, _1, _2));
		gl.addProcedure("layout-add-splitter", std::bind(&user_interface::layout_add_splitter, &itf, _1, _2));
		gl.addProcedure("layout-get-splitter-count", std::bind(&user_interface::layout_get_splitters_count, &itf, _1, _2));
		gl.addProcedure("layout-remove-splitter", std::bind(&user_interface::layout_remove_splitter, &itf, _1, _2));
		gl.addProcedure("tabs-add-component", std::bind(&user_interface::tabs_add_component, &itf, _1, _2));
		gl.addProcedure("set-main-component", std::bind(&user_interface::set_main_component, &itf, _1, _2));
		gl.addProcedure("refresh-interface", std::bind(&user_interface::refresh_interface, &itf, _1, _2));

		// GUI event binding
		gl.addProcedure("bind-mouse-click", std::bind(&user_interface::bind_mouse_listener<mouseUpListener>, &itf, _1, _2));
		gl.addProcedure("bind-mouse-double-click", std::bind(&user_interface::bind_mouse_listener<mouseDoubleClickListener>, &itf, _1, _2));
		gl.addProcedure("unbind-mouse", std::bind(&user_interface::unbind_mouse_listener, &itf, _1, _2));

		gl.addProcedure("playlist-get-selected", std::bind(&user_interface::playlist_get_selected, &itf, _1, _2));

		// playback API
		gl.addProcedure("playback-set-file", std::bind(&playback::set_file, std::ref(gl), _1, _2));
		gl.addProcedure("playback-unload-file", std::bind(&playback::unload_file, std::ref(gl), _1, _2));
		gl.addProcedure("playback-start", std::bind(&playback::start, std::ref(gl), _1, _2));
		gl.addProcedure("playback-stop", std::bind(&playback::stop, std::ref(gl), _1, _2));
		gl.addProcedure("playback-seek", std::bind(&playback::seek, std::ref(gl), _1, _2));
		gl.addProcedure("playback-length", std::bind(&playback::length, std::ref(gl), _1, _2));
		gl.addProcedure("playback-get-pos", std::bind(&playback::get_pos, std::ref(gl), _1, _2));
		gl.addProcedure("playback-is-playing", std::bind(&playback::is_playing, std::ref(gl), _1, _2));

		// test
		gl.eval("(defun on-playlist-click (item-str) (playback-set-file item-str) (playback-start) )");

		gl.eval("(create-playlist 'p1)");
		gl.eval("(create-playlist 'p2)");
		gl.eval("(create-layout 'l1 t)");
		gl.eval("(layout-add-component 'l1 'p1 -0.1 -0.9 -0.5)");
		gl.eval("(layout-add-splitter 'l1)");
		gl.eval("(layout-add-component 'l1 'p2 -0.1 -0.9 -0.5)");
		gl.eval("(create-layout 'l2 nil)");
		gl.eval("(create-interpreter 'int1)");
		gl.eval("(layout-add-component 'l2 'int1 50.0 100.0 50.0)");
		gl.eval("(layout-add-splitter 'l2)");
		gl.eval("(layout-add-component 'l2 'l1 -0.1 -1.0 -0.9)");
		gl.eval("(set-main-component 'l2)");
		gl.eval("(create-tabs 'tab 'bottom)");
		gl.eval("(create-playlist 'plpl)");
		gl.eval("(layout-add-component 'l2 'tab 300.0 300.0 300.0)");
		gl.eval("(defvar cl-red |1.0 0.0 0.0 1.0|)");
		gl.eval("(tabs-add-component 'tab 'plpl \"First tab\" cl-red)");
		gl.eval("(create-audio-settings 'sss)");
		gl.eval("(tabs-add-component 'tab 'sss \"Audio Settings\" |0.0 0.8 0.0 0.5|)");
		gl.eval("(defun on-play-clicked () (layout-remove-splitter 'l1 0) (refresh-interface))");
		gl.eval("(tabs-add-component 'tab (create-text-button 'playb \"Play\" \"Plays selected track\") \"Playback API\" |0.0 0.1 0.5 0.9|)");
		gl.eval("(bind-mouse-click 'playb 'on-play-clicked)");
		gl.eval("(refresh-interface)");
	}

	void closeButtonPressed() override {
		gl.close();
		playback::shutdown();
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
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override {
    }
};

START_JUCE_APPLICATION(KiwanoApplication);

// TODO: load config from file
