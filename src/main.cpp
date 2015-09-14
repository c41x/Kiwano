#include <AppConfig.h>
// #include <modules/juce_audio_basics/juce_audio_basics.h>
// #include <modules/juce_audio_devices/juce_audio_devices.h>
// #include <modules/juce_audio_formats/juce_audio_formats.h>
// #include <modules/juce_audio_processors/juce_audio_processors.h>
#include <modules/juce_core/juce_core.h>
// #include <modules/juce_cryptography/juce_cryptography.h>
// #include <modules/juce_data_structures/juce_data_structures.h>
// #include <modules/juce_events/juce_events.h>
// #include <modules/juce_graphics/juce_graphics.h>
#include <modules/juce_gui_basics/juce_gui_basics.h>
// #include <modules/juce_gui_extra/juce_gui_extra.h>
// #include <modules/juce_opengl/juce_opengl.h>
// #include <modules/juce_video/juce_video.h>
#include <base/base.hpp>
#include <memory>

using namespace juce;
using namespace granite;

namespace ProjectInfo
{
    const char* const  projectName    = "Kiwano";
    const char* const  versionString  = "0.1";
    const int          versionNumber  = 0x10000;
}

class playlist : public Component, public FileDragAndDropTarget {
    struct playlistModel : public ListBoxModel {
		StringArray entries;
        int getNumRows() override {
            return entries.size();
        }

        void paintListBoxItem(int rowNumber, Graphics& g,
							  int width, int height, bool rowIsSelected) override {
            if (rowIsSelected)
                g.fillAll(Colours::lightblue);

            g.setColour(Colours::black);
            g.setFont(height * 0.7f);
			g.drawText(entries[rowNumber], 5, 0, width, height, Justification::centredLeft, true);
        }
    };

    ListBox box;
    playlistModel model;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(playlist);

public:
    playlist() : box("playlist-box", nullptr) {
		setName("playlist");
		box.setModel(&model);
		box.setMultipleSelectionEnabled(true);
		addAndMakeVisible(box);
	}

    void resized() override {
		box.setBounds(getLocalBounds().reduced(0));
	}

	bool isInterestedInFileDrag(const StringArray& /*files*/) override {
		return true;
	}

	void fileDragEnter(const StringArray& /*files*/, int /*x*/, int /*y*/) override {
		repaint();
	}

	void fileDragMove (const StringArray& /*files*/, int /*x*/, int /*y*/) override {}

	void fileDragExit (const StringArray& /*files*/) override {
		repaint();
	}

	void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override {
		model.entries.addArray(files);
		box.updateContent();
		repaint();
	}
};

class tabs : public TabbedComponent {
public:
    tabs() : TabbedComponent(TabbedButtonBar::TabsAtTop) {
        addTab("Menus", getRandomTabBackgroundColour(), new playlist(), true);
        addTab("Buttons", getRandomTabBackgroundColour(), new playlist(), true);
    }

    static Colour getRandomTabBackgroundColour() {
        return Colour(Random::getSystemRandom().nextFloat(), 0.1f, 0.97f, 1.0f);
    }
};

class layout : public Component {
	StretchableLayoutManager l;
	std::vector<std::unique_ptr<Component>> components;
	std::vector<Component*> lc;
	bool horizontal;

public:
	layout(bool _horizontal) : horizontal(_horizontal){
		setOpaque(true);
	}

	void paint(Graphics &g) override {
		g.setColour(Colour::greyLevel(0.2f));
		g.fillAll();
	}

	void resized() override {
		juce::Rectangle<int> r(getLocalBounds());
		l.layOutComponents(lc.data(), lc.size(), r.getX(), r.getY(), r.getWidth(), r.getHeight(), !horizontal, true);
	}

	void addSplitter() {
		if (lc.size() > 0) {
			l.setItemLayout(lc.size(), 5, 5, 5);
			components.push_back(std::make_unique<StretchableLayoutResizerBar>(&l, lc.size(), horizontal));
			lc.push_back(components.back().get());
			addAndMakeVisible(components.back().get());
		}
	}

	void addComponent(Component *c, double minimum, double maximum, double preferred) {
		l.setItemLayout(lc.size(), minimum, maximum, preferred);
		lc.push_back(c);
		addAndMakeVisible(c);
	}
};

class interpreter : public Component,
					public TextEditor::Listener {
	base::lisp &gl;
	TextEditor te;

public:
	interpreter(base::lisp &glisp) : gl(glisp) {
		te.setMultiLine(true);
		te.setReturnKeyStartsNewLine(false);
		te.setTabKeyUsedAsCharacter(false);
		te.setReadOnly(false);
		te.setCaretVisible(true);
		te.addListener(this);
		addAndMakeVisible(te);
	}
	~interpreter() {}

	void resized() override {
		te.setBounds(getLocalBounds());
	}

    void textEditorTextChanged(TextEditor&) override {}
    void textEditorEscapeKeyPressed(TextEditor&) override {}
    void textEditorFocusLost(TextEditor&) override {}

    void textEditorReturnKeyPressed(TextEditor&) override {
		// get line
		te.moveCaretToStartOfLine(false);
		int begin = te.getCaretPosition();
		te.moveCaretToEndOfLine(false);
		String t = te.getTextInRange(Range<int>(begin, te.getCaretPosition()));

		// execute code and print result
		std::string r = gl.eval(t.toStdString());
		te.insertTextAtCaret("\n > ");
		te.insertTextAtCaret(r);
		te.insertTextAtCaret("\n");
    }
};

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

class KiwanoApplication : public JUCEApplication {
    class MainWindow : public DocumentWindow {
		user_interface itf;
		base::lisp gl;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)

		public:
		MainWindow(String name) : DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons),
			itf(gl) {
			setContentOwned(&itf, false);
			setSize(800, 600);
			setTopLeftPosition(200, 200);
            setVisible(true);
			setUsingNativeTitleBar(true);
			setResizable(true, true);

			// initialize GLISP
			using namespace std::placeholders;
			gl.init();
			gl.addProcedure("create-playlist", std::bind(&user_interface::create_playlist, &itf, _1, _2));
			gl.addProcedure("create-layout", std::bind(&user_interface::create_layout, &itf, _1, _2));
			gl.addProcedure("layout-add-component", std::bind(&user_interface::layout_add_component, &itf, _1, _2));
			gl.addProcedure("layout-add-splitter", std::bind(&user_interface::layout_add_splitter, &itf, _1, _2));
			gl.addProcedure("set-main-component", std::bind(&user_interface::set_main_component, &itf, _1, _2));
			gl.addProcedure("create-interpreter", std::bind(&user_interface::create_interpreter, &itf, _1, _2));
			gl.addProcedure("refresh-interface", std::bind(&user_interface::refresh_interface, &itf, _1, _2));
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
        }

        void closeButtonPressed() override {
			gl.close();
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

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

START_JUCE_APPLICATION(KiwanoApplication)
