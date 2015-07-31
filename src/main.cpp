#include <AppConfig.h>
#include <modules/juce_audio_basics/juce_audio_basics.h>
#include <modules/juce_audio_devices/juce_audio_devices.h>
#include <modules/juce_audio_formats/juce_audio_formats.h>
#include <modules/juce_audio_processors/juce_audio_processors.h>
#include <modules/juce_core/juce_core.h>
#include <modules/juce_cryptography/juce_cryptography.h>
#include <modules/juce_data_structures/juce_data_structures.h>
#include <modules/juce_events/juce_events.h>
#include <modules/juce_graphics/juce_graphics.h>
#include <modules/juce_gui_basics/juce_gui_basics.h>
#include <modules/juce_gui_extra/juce_gui_extra.h>
#include <modules/juce_opengl/juce_opengl.h>
#include <modules/juce_video/juce_video.h>

using namespace juce;
namespace ProjectInfo
{
    const char* const  projectName    = "Kiwano";
    const char* const  versionString  = "0.1";
    const int          versionNumber  = 0x10000;
}

class KiwanoApplication  : public JUCEApplication
{
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

    class MainWindow : public DocumentWindow {
    public:
        MainWindow(String name) : DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons) {
            centreWithSize(300, 200);
            setVisible(true);
        }

        void closeButtonPressed() override {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    ScopedPointer<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(KiwanoApplication)
