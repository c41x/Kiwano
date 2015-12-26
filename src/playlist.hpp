#pragma once
#include "includes.hpp"

#define TAGLIB_STATIC
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

class playlist : public Component, public FileDragAndDropTarget {
    struct playlistModel : public TableListBoxModel {
		base::string paths;
		std::vector<size_t> paths_i;

		void init() {
			paths.clear();
			paths_i.clear();
			paths_i.push_back(0);
		}

		void addItem(const String &path) {

			//TagLib::FileRef file(f.toRawUTF8());
			//f = (file.tag()->album() + L" | " + file.tag()->artist() + L" | " + file.tag()->title()).toCString();

			paths.append(path.toStdString());
			paths_i.push_back(paths.size());
		}

		base::string getItemPath(size_t index) {
			return base::string(paths.begin() + paths_i[index],
								paths.begin() + paths_i[index + 1]);
		}

		// GUI
        int getNumRows() override {
            return paths_i.size() - 1;
        }

		// This is overloaded from TableListBoxModel, and should fill in the background of the whole row
		void paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override {
			if (rowIsSelected)
				g.fillAll(Colours::lightblue);
			else if (rowNumber % 2)
				g.fillAll(Colour(0xffeeeeee));
		}

		// This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
		// components.
		void paintCell(Graphics& g, int rowNumber, int /*columnId*/,
						int width, int height, bool /*rowIsSelected*/) override {
			g.setColour(Colours::black);
			g.setFont(height * 0.7f);
			g.drawText(getItemPath(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
			g.setColour(Colours::black.withAlpha(0.2f));
			g.fillRect(width - 1, 0, 1, height);
		}
    };

    TableListBox box;
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
		model.init();
		setName("playlist");
		box.setModel(&model);
		box.setMultipleSelectionEnabled(true);
		addAndMakeVisible(box);

		box.getHeader().addColumn("album", 0, 200, 50, 1000, TableHeaderComponent::defaultFlags);
		box.getHeader().addColumn("artist", 0, 200, 50, 1000, TableHeaderComponent::defaultFlags);
		box.getHeader().addColumn("title", 0, 200, 50, 1000, TableHeaderComponent::defaultFlags);
		box.setMultipleSelectionEnabled(true);
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
				model.addItem(f);
			}
		}
		box.updateContent();
		repaint();
	}

	base::string getSelectedRowString() {
		return model.getItemPath(box.getSelectedRow());
	}
};
