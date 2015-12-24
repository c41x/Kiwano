#pragma once
#include "includes.hpp"

#define TAGLIB_STATIC
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

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

	// TODO: system codecs?
	bool isFileSupported(const String &fname) {
		return fname.endsWith(".mp3")
			|| fname.endsWith(".wav")
			|| fname.endsWith(".wma")
			|| fname.endsWith(".flac")
			|| fname.endsWith(".ogg")
			|| fname.endsWith(".ape");
	}

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
		for (auto &f : files) {
			if (isFileSupported(f)) {
				//TagLib::FileRef file(f.toRawUTF8());
				//f = (file.tag()->album() + L" | " + file.tag()->artist() + L" | " + file.tag()->title()).toCString();
			}
		}
		model.entries.addArray(files);
		box.updateContent();
		repaint();
	}

	base::string getSelectedRowString() {
		return model.entries[box.getSelectedRow()].toStdString();
	}
};
