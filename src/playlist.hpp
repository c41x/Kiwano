#pragma once
#include "includes.hpp"
#include "customTags.hpp"
#include "playlistModel.hpp"
#include "playlistProgress.hpp"
#include "seekRange.hpp"

class playlist : public Component, public FileDragAndDropTarget {
    TableListBox box;
    playlistModel model;
    base::lisp &gl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(playlist);

    void init(float height = 18.0f) {
        box.setModel(&model);
        box.setMultipleSelectionEnabled(true);
        box.getHeader().setStretchToFitActive(true);
        box.setHeaderHeight(0.0f); // hide header for now
        box.setRowHeight(height);
        addAndMakeVisible(box);
    }

public:

    playlist(base::lisp &_gl, base::string name, float height)
            : box("playlist-box", nullptr),
              model(_gl, name),
              gl(_gl) {
        model.init();
        setComponentID(name);
        init(height);
    }

    playlist(playlist &r, const base::string &filter, base::lisp &_gl, const base::string &name)
            : box("playlist-box", nullptr),
              model(r.model, filter, _gl, name),
              gl(_gl) {
        setComponentID(name);
        init();
    }

    void resized() override {
        box.setBounds(getLocalBounds());
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
        (new playlistProgress(model, files, [this](){
                box.updateContent();
                repaint();
            }))->launchThread();
    }

    base::string getSelectedRowPath() const {
        return getRowPath(box.getSelectedRow());
    }

    base::string getSelectedRowPathRaw() const {
        return model.getItemPath(box.getSelectedRow());
    }

    base::string getSelectedRowId() const {
        return model.getItemId(box.getSelectedRow());
    }

    int32 getSelectedRowIndex() const {
        return box.getSelectedRow();
    }

    void selectRow(int32 row) {
        box.selectRow(row);
    }

    // gets path with cue timestamps
    base::string getRowPath(int32 i) const {
        auto seek = getRowSeek(i);

        if (!seek.empty())
            return base::strs(getRowPathRaw(i), ":", seek.toString());

        return getRowPathRaw(i);
    }

    // gets file path only
    base::string getRowPathRaw(int32 i) const { return model.getItemPath(i); }

    base::string getRowId(int32 i) const { return model.getItemId(i); }

    seekRange getRowSeek(int32 i) const { return model.getItemSeek(i); }

    bool isTrack(int32 i) const { return model.isTrack(i); }

    base::string getRowAlbum(int32 i) const { return model.getItemAlbum(i); }
    base::string getRowArtist(int32 i) const { return model.getItemArtist(i); }
    base::string getRowTitle(int32 i) const { return model.getItemTitle(i); }
    int32 getRowYear(int32 i) const { return model.getItemYear(i); }
    int32 getRowTrack(int32 i) const { return model.getItemTrack(i); }

    int32 getItemsCount() const {
        return model.getItemsCount();
    }

    bool setColor(const base::string &id, Colour color) {
        return model.setColor(id, color);
    }

    void addColumn(const base::string &caption, const base::string &content,
                   const base::string &contentGroup, int width, int widthMin, int widthMax) {
        box.getHeader().addColumn(caption, (int)model.columns.size(),
                                  width, widthMin, widthMax, TableHeaderComponent::defaultFlags);
        model.columns.push_back(content);
        model.columnsGroup.push_back(contentGroup);
    }

    void filterEnable(const base::string &query) {
        model.filterEnable(query);
        box.repaint();
    }

    void filterDisable() {
        model.filterDisable();
        box.repaint();
    }

    bool filterEnabled() const {
        return model.filterEnabled;
    }

    void filterSelectNext() {
        int current = box.getSelectedRow();
        if (current == -1)
            current = 0; // no row selected -> start from the beginning
        int next = model.filterNextIndex(current, true); // search for next index
        if (current != next) {
            box.selectRow(next);
        }
    }

    bool store(const base::string &f) {
        base::stream s;
        s.write(model.paths);
        s.write(model.talbum);
        s.write(model.tartist);
        s.write(model.ttitle);
        s.write(model.tyear);
        s.write(model.tseek);
        s.write(model.ttrack);
        s.write(model.tlength);
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
            && s.read(model.tseek) > 0
            && s.read(model.ttrack) > 0
            && s.read(model.tlength) > 0
            && s.read(model.paths_i) > 0
            && s.read(model.talbum_i) > 0
            && s.read(model.tartist_i) > 0
            && s.read(model.ttitle_i) > 0;
        box.updateContent();
        repaint();
        return result;
    }
};

// TODO: remove-column
// TODO: clear-columns
// TODO: get-columns
// TODO: deleting subsection
// TODO: make sorters
