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
		box.setBounds(getLocalBounds().reduced(8));
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
	StretchableLayoutResizerBar rb;
	std::vector<Component *> components;

public:
	layout(Component *c1, Component *c2) : rb(&l, 1, false) {
		setOpaque(true);
		addAndMakeVisible(rb);
		components.push_back(c1);
		components.push_back(c2);
		components.push_back(nullptr);
		l.setItemLayout(0, -0.1, -0.9, -0.5);
		l.setItemLayout(1, -0.1, -0.9, -0.5);
	}

	void resized() {
        Rectangle<int> r(getLocalBounds().reduced(4));
		l.layOutComponents(components.data(), 2, r.getX(), r.getY(), r.getWidth(), r.getHeight(), true, true);
	}
};

class KiwanoApplication : public JUCEApplication {
    class MainWindow : public DocumentWindow {
		layout l;
		tabs c1, c2;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)

    public:
        MainWindow(String name) : DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons),
			l(&c1, &c2) {
			addAndMakeVisible(&l);
			setContentOwned(&l, true);
            setVisible(true);
			setSize(600, 400);
			l.setSize(500, 300);
        }

        void closeButtonPressed() override {
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
