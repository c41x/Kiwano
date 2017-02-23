#pragma once
#include "includes.hpp"
#include "playlistModel.hpp"

// progress dialog
class playlistProgress : public ThreadWithProgressWindow {
    playlistModel &m;
    StringArray files;
    std::function<void()> onFinish;
public:
    playlistProgress(playlistModel &model, const StringArray &_files, std::function<void()> _onFinish)
            : ThreadWithProgressWindow("Importing music", true, true),
              m(model), files(_files), onFinish(_onFinish) {
        setStatusMessage("Importing music");
    }

    // TODO: system codecs? add-accepted-format?
    bool isFileSupported(const String &fname) {
        return fname.endsWith(".mp3")
            || fname.endsWith(".wav")
            || fname.endsWith(".wma")
            || fname.endsWith(".flac")
            || fname.endsWith(".ogg")
            || fname.endsWith(".ape")
            || fname.endsWith(".wv")
            || fname.endsWith(".cue");
    }

    void rscan(const String &fileName, int &nth) {
        // scan for files in current directory
        std::vector<String> directoryContents;
        DirectoryIterator fi(File(fileName), false, "*", File::findFiles);
        while (fi.next()) {
            if (isFileSupported(fi.getFile().getFileName())) {
                directoryContents.push_back(fi.getFile().getFullPathName());
                ++nth;
            }
        }

        m.addItemGroup(directoryContents);

        // scan for subdirectories
        DirectoryIterator di(File(fileName), false, "*", File::findDirectories);
        while (di.next()) {
            rscan(di.getFile().getFullPathName(), nth);
        }

        // update progress bar
        if (nth > 50) {
            nth = 0;
            setStatusMessage(fileName);
        }
    }

    void run() override {
        setProgress(-1.0);

        // files first
        std::vector<String> directoryContents;
        int nth = 0;

        for (auto &fileName : files) {
            if (!File(fileName).isDirectory()) {
                if (isFileSupported(fileName)) {
                    directoryContents.push_back(fileName);
                    ++nth;
                }
            }
        }

        m.addItemGroup(directoryContents);

        // then directories
        for (auto &fileName : files) {
            if (File(fileName).isDirectory()) {
                rscan(fileName, nth);
            }
        }

        setStatusMessage("Done.");
    }

    void threadComplete(bool/* userPressedCancel*/) override {
        onFinish();
        delete this;
    }
};
