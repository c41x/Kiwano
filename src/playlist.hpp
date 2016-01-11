#pragma once
#include "includes.hpp"

#define TAGLIB_STATIC
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

class playlist : public Component, public FileDragAndDropTarget {
    struct playlistModel : public TableListBoxModel {
		base::string paths, talbum, tartist, ttitle;
		std::vector<uint32> tyear, ttrack;
		std::vector<uint32> paths_i, talbum_i, tartist_i, ttitle_i;

		void init() {
			paths_i.clear(); // this one first (getNumRows)
			paths_i.push_back(0);
			paths.clear();
			talbum.clear();
			tartist.clear();
			tyear.clear();
			ttitle.clear();
			ttrack.clear();
			talbum_i.clear();
			talbum_i.push_back(0);
			tartist_i.clear();
			tartist_i.push_back(0);
			ttitle_i.clear();
			ttitle_i.push_back(0);
		}

		void addItem(const String &path) {
			paths.append(path.toStdString());
			paths_i.push_back(paths.size());

			// read tags from file
			TagLib::FileRef file(path.toRawUTF8());
			if (!file.isNull() && file.tag()) {
				talbum.append(file.tag()->album().toCString());
				tartist.append(file.tag()->artist().toCString());
				ttitle.append(file.tag()->title().toCString());
				tyear.push_back(file.tag()->year());
				ttrack.push_back(file.tag()->track());
			}
			else {
				tyear.push_back(0);
				ttrack.push_back(0);
			}
			talbum_i.push_back(talbum.size());
			tartist_i.push_back(tartist.size());
			ttitle_i.push_back(ttitle.size());
		}

		base::string getItemPath(size_t index) {
			return base::string(paths.begin() + paths_i[index],
								paths.begin() + paths_i[index + 1]);
		}

		uint getItemTrack(size_t index) {
			return ttrack[index];
		}

		uint getItemYear(size_t index) {
			return tyear[index];
		}

		base::string getItemAlbum(size_t index) {
			return base::string(talbum.begin() + talbum_i[index],
								talbum.begin() + talbum_i[index + 1]);
		}

		base::string getItemArtist(size_t index) {
			return base::string(tartist.begin() + tartist_i[index],
								tartist.begin() + tartist_i[index + 1]);
		}

		base::string getItemTitle(size_t index) {
			return base::string(ttitle.begin() + ttitle_i[index],
								ttitle.begin() + ttitle_i[index + 1]);
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
		void paintCell(Graphics& g, int rowNumber, int columnId,
						int width, int height, bool /*rowIsSelected*/) override {
			g.setColour(Colours::black);
			g.setFont(height * 0.7f);
			if (columnId == 0)
				g.drawText(base::toStr(getItemTrack(rowNumber)), 5, 0, width, height, Justification::centredLeft, true);
			else if (columnId == 1)
				g.drawText(getItemAlbum(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
			else if (columnId == 2)
				g.drawText(getItemArtist(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
			else if (columnId == 3)
				g.drawText(getItemTitle(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
			else if (columnId == 4)
				g.drawText(base::toStr(getItemYear(rowNumber)), 5, 0, width, height, Justification::centredLeft, true);
			g.setColour(Colours::black.withAlpha(0.2f));
			g.fillRect(width - 1, 0, 1, height);
		}
    };

	// progress dialog
	class progress : public ThreadWithProgressWindow {
		playlistModel &m;
		StringArray files;
		std::function<void()> onFinish;
	public:
		progress(playlistModel &model, const StringArray &_files, std::function<void()> _onFinish)
				: ThreadWithProgressWindow("Importing music", true, true),
				  m(model), files(_files), onFinish(_onFinish) {
			setStatusMessage("Importing music");
		}

		// TODO: system codecs?
		bool isFileSupported(const String &fname) {
			return fname.endsWith(".mp3")
				|| fname.endsWith(".wav")
				|| fname.endsWith(".wma")
				|| fname.endsWith(".flac")
				|| fname.endsWith(".ogg")
				|| fname.endsWith(".ape");
		}

		void run() override {
			setProgress(-1.0);
			int nth = 0;
			for (auto &fileName : files) {
				if (File(fileName).isDirectory()) {
					// recursively scan for files
					DirectoryIterator i(File(fileName), true, "*.mp3;*.wav;*.wma;*.flac;*.ogg;*.ape");
					while (i.next()) {
						m.addItem(i.getFile().getFullPathName());
						if (nth++ == 50) {
							nth = 0;
							setStatusMessage(i.getFile().getFullPathName());
						}
					}
				}
				else {
					// single file
					if (isFileSupported(fileName))
						m.addItem(fileName);
				}
			}
			setStatusMessage("Done.");
		}

		void threadComplete(bool/* userPressedCancel*/) override {
			onFinish();
			delete this;
		}
	};

    TableListBox box;
    playlistModel model;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(playlist);

public:
    playlist() : box("playlist-box", nullptr) {
		model.init();
		box.setModel(&model);
		box.setMultipleSelectionEnabled(true);
		box.getHeader().setStretchToFitActive(true);
		addAndMakeVisible(box);

		box.getHeader().addColumn("track", 0, 50, 20, 1000, TableHeaderComponent::defaultFlags);
		box.getHeader().addColumn("album", 1, 200, 150, 1000, TableHeaderComponent::defaultFlags);
		box.getHeader().addColumn("artist", 2, 200, 150, 1000, TableHeaderComponent::defaultFlags);
		box.getHeader().addColumn("title", 3, 200, 150, 1000, TableHeaderComponent::defaultFlags);
		box.getHeader().addColumn("year", 4, 70, 50, 1000, TableHeaderComponent::defaultFlags);
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
		(new progress(model, files, [this](){
				box.updateContent();
				repaint();
			}))->launchThread();
	}

	base::string getSelectedRowString() {
		return model.getItemPath(box.getSelectedRow());
	}

	bool store(const base::string &f) {
		base::stream s;
		s.write(model.paths);
		s.write(model.talbum);
		s.write(model.tartist);
		s.write(model.ttitle);
		s.write(model.tyear);
		s.write(model.ttrack);
		s.write(model.paths_i);
		s.write(model.talbum_i);
		s.write(model.tartist_i);
		s.write(model.ttitle_i);
		return base::fs::store(f, s);
	}

	bool load(const base::string &f) {
		base::stream s = base::fs::load(f);
		bool result = s.read(model.paths) > 0
			&& s.read(model.talbum) > 0
			&& s.read(model.tartist) > 0
			&& s.read(model.ttitle) > 0
			&& s.read(model.tyear) > 0
			&& s.read(model.ttrack) > 0
			&& s.read(model.paths_i) > 0
			&& s.read(model.talbum_i) > 0
			&& s.read(model.tartist_i) > 0
			&& s.read(model.ttitle_i) > 0;
		box.updateContent();
		repaint();
		return result;
	}
};
