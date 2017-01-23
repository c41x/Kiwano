#pragma once
#include "includes.hpp"
#include "seekRange.hpp"

#define TAGLIB_STATIC
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

extern "C" {
    #ifdef GE_PLATFORM_WINDOWS
    #include <libcue.h>
    #else
    #include <libcue/libcue.h>
    #endif
}

struct playlistModel : public TableListBoxModel {
    base::string paths, talbum, tartist, ttitle;
    std::vector<seekRange> tseek;
    std::vector<uint32> tyear, ttrack;
    std::vector<uint32> paths_i, talbum_i, tartist_i, ttitle_i;
    std::vector<base::string> columns;

    // filtering
    base::string filterQuery;
    bool filterEnabled;

    playlistModel(playlistModel &r, const base::string &filter) {
        // reset
        paths_i = {0};
        talbum_i = {0};
        tartist_i = {0};
        ttitle_i = {0};
        init();

        // fill (filter)
        const int items = r.getItemsCount();
        base::string f = base::lowerCase(filter);
        for (int i = 0; i < items; ++i) {
            if (filterMatch(i, f)) {
                paths.append(r.getItemPath(i));
                talbum.append(r.getItemAlbum(i));
                tartist.append(r.getItemArtist(i));
                ttitle.append(r.getItemTitle(i));
                tyear.push_back(r.getItemYear(i));
                tseek.push_back(r.getItemSeek(i));
                ttrack.push_back(r.getItemTrack(i));
                paths_i.push_back(paths.size());
                talbum_i.push_back(talbum.size());
                tartist_i.push_back(tartist.size());
                ttitle_i.push_back(ttitle.size());
            }
        }
    }

    playlistModel() : filterEnabled(false) {}
    ~playlistModel() {}

    void init() {
        paths_i.clear(); // this one first (getNumRows)
        paths_i.push_back(0);
        paths.clear();
        talbum.clear();
        tartist.clear();
        tyear.clear();
        tseek.clear();
        ttitle.clear();
        ttrack.clear();
        talbum_i.clear();
        talbum_i.push_back(0);
        tartist_i.clear();
        tartist_i.push_back(0);
        ttitle_i.clear();
        ttitle_i.push_back(0);
    }

    void addItemGroup(const std::vector<String> &group) {
        for (const auto &e : group) {
            addItem(e);
        }
    }

    void addItem(const String &path) {
        base::string gpath = path.toStdString();
        base::string basePath = base::extractFilePath(gpath);
        if (base::extractExt(gpath) == "cue") {
            base::stream s = base::fs::load(gpath);
            if (s.size() > 0) {
                Cd *cd = cue_parse_string((const char*)s.data());
                if (cd != nullptr) {
                    Rem *rem = cd_get_rem(cd);
                    if (rem != nullptr) {
                        Cdtext *cdtext = cd_get_cdtext(cd);
                        if (cdtext != nullptr) {
                            const char *defaultArtist = cdtext_get(PTI_PERFORMER, cdtext);
                            const char *defaultTitle = cdtext_get(PTI_TITLE, cdtext);
                            const char *defaultDate = rem_get(REM_DATE, rem);
                            int tracks = cd_get_ntrack(cd);
                            const char *path = nullptr;
                            for (int i = 1; i <= tracks; ++i) {
                                Track *track = cd_get_track(cd, i);
                                path = track_get_filename(track); // if diff -> store new?
                                cdtext = track_get_cdtext(track);
                                const char *artist = nullptr;
                                const char *title = nullptr;
                                int trackIndex = -1;
                                int start = 0;
                                int length = 0;
                                const char *date = nullptr;

                                if (cdtext != nullptr) {
                                    artist = cdtext_get(PTI_PERFORMER, cdtext);
                                    title = cdtext_get(PTI_TITLE, cdtext);
                                    trackIndex = track_get_index(track, i);
                                    start = track_get_start(track);
                                    length = track_get_length(track);
                                }
                                rem = track_get_rem(track);
                                if (rem != nullptr) {
                                    date = rem_get(REM_DATE, rem);
                                }

                                if (defaultArtist == nullptr) defaultArtist = "?";
                                if (defaultTitle == nullptr) defaultTitle = "?";
                                if (defaultDate == nullptr) defaultDate = "?";
                                if (path == nullptr) path = "?";
                                if (artist == nullptr) artist = "?";
                                if (title == nullptr) title = "?";
                                if (date == nullptr) date = "?";

                                paths.append(base::strs(basePath, GE_DIR_SEPARATOR, path)); // prevent duplicates?
                                paths_i.push_back(paths.size());
                                talbum.append(defaultTitle);
                                tartist.append(artist);
                                ttitle.append(title);
                                if (base::strIs<int>(date))
                                    tyear.push_back(base::fromStr<int>(date));
                                else if (base::strIs<int>(defaultDate))
                                    tyear.push_back(base::fromStr<int>(defaultDate));
                                else tyear.push_back(0);
                                tseek.push_back({start, start + length});
                                if (trackIndex < 0)
                                    ttrack.push_back(i);
                                else ttrack.push_back(trackIndex + 1);
                                talbum_i.push_back(talbum.size());
                                tartist_i.push_back(tartist.size());
                                ttitle_i.push_back(ttitle.size());
                            }
                            return;
                        }
                    }
                }
                logInfo(base::strs("CUE: could not parse / syntax error in: ", gpath));
            }
            else {
                logInfo(base::strs("could not load CUE - empty file: ", gpath));
            }
        }
        else {
            paths.append(gpath);
            paths_i.push_back(paths.size());

            // read tags from file
            TagLib::FileRef file(path.toRawUTF8());
            if (!file.isNull() && file.tag()) {
                talbum.append(file.tag()->album().toCString());
                tartist.append(file.tag()->artist().toCString());
                ttitle.append(file.tag()->title().toCString());
                tyear.push_back(file.tag()->year());
                tseek.push_back(seekRange());
                ttrack.push_back(file.tag()->track());
            }
            else {
                tyear.push_back(0);
                tseek.push_back(seekRange());
                ttrack.push_back(0);
            }
            talbum_i.push_back(talbum.size());
            tartist_i.push_back(tartist.size());
            ttitle_i.push_back(ttitle.size());
        }
    }

    // checks if given query match item at index
    bool filterMatch(size_t index, const base::string &query) const {
        base::string f = base::lowerCase(query);
        return base::string::npos != base::lowerCase(getItemArtist(index)).find(f)
            || base::string::npos != base::lowerCase(getItemAlbum(index)).find(f)
            || base::string::npos != base::lowerCase(getItemTitle(index)).find(f);
    }

    // returns next filtered item index
    int filterNextIndex(int currentPosition, bool wrap) const {
        const int items = getItemsCount();
        for (int i = currentPosition + 1; i < items; ++i) {
            if (filterMatch(i, filterQuery))
                return i;
        }

        // wrapped search
        if (wrap)
            return filterNextIndex(0, false);

        // no result found - return currentPosition back
        return currentPosition;
    }

    // enables highlight for filter
    void filterEnable(const base::string &query) {
        filterEnabled = true;
        filterQuery = query;

    }

    void filterDisable() {
        filterEnabled = false;
    }

    base::string getItemPath(size_t index) const {
        return base::string(paths.begin() + paths_i[index],
                            paths.begin() + paths_i[index + 1]);
    }

    uint getItemTrack(size_t index) const {
        return ttrack[index];
    }

    uint getItemYear(size_t index) const {
        return tyear[index];
    }

    seekRange getItemSeek(size_t index) const {
        return tseek[index];
    }

    base::string getItemAlbum(size_t index) const {
        return base::string(talbum.begin() + talbum_i[index],
                            talbum.begin() + talbum_i[index + 1]);
    }

    base::string getItemArtist(size_t index) const {
        return base::string(tartist.begin() + tartist_i[index],
                            tartist.begin() + tartist_i[index + 1]);
    }

    base::string getItemTitle(size_t index) const {
        return base::string(ttitle.begin() + ttitle_i[index],
                            ttitle.begin() + ttitle_i[index + 1]);
    }

    base::string getItemId(size_t index) const {
        return getItemAlbum(index) + getItemArtist(index) + getItemTitle(index) + base::toStr(getItemTrack(index));
    }

    // GUI
    int getItemsCount() const {
        return paths_i.size() - 1 + 2;
    }

    int getNumRows() override {
        return getItemsCount();
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override {
        if (filterEnabled && filterMatch(rowNumber, filterQuery)) {
            if (rowIsSelected)
                g.fillAll(Colours::lightyellow);
            else g.fillAll(Colours::lightgreen);
        }
        else if (rowIsSelected) {
            g.fillAll(Colours::lightblue);
        }
        else if (rowNumber % 2) {
            g.fillAll(Colour(0xffeeeeee));
        }
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell(Graphics& g, int rowNumber, int columnId,
                   int width, int height, bool /*rowIsSelected*/) override {
        g.setColour(Colours::black);
        g.setFont(juce::Font("Ubuntu Condensed", height * 0.9f, juce::Font::plain));

        if (rowNumber == 3 || rowNumber == 20) {
            g.setColour(Colours::black.withAlpha(0.2f));
            g.fillRect(0, height / 2 - 1, width, 2);
            return;
        }

        if (rowNumber > 20) {
            rowNumber--;
            rowNumber--;
        }
        else if (rowNumber > 3) {
            rowNumber--;
        }

        if (columnId < (int)columns.size() &&
            rowNumber < (getNumRows() - 2)) {
            auto &c = columns[columnId];
            if (c == "track")
                //g.drawText(base::toStr(getItemTrack(rowNumber)), 5, 0, width, height, Justification::centredLeft, true);
                g.drawText(base::toStr(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
            else if (c == "album")
                g.drawText(getItemAlbum(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
            else if (c == "artist")
                g.drawText(getItemArtist(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
            else if (c == "title")
                g.drawText(getItemTitle(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
            else if (c == "year")
                g.drawText(base::toStr(getItemYear(rowNumber)), 5, 0, width, height, Justification::centredLeft, true);
            else {
                // custom tag
                if (base::strIs<int>(c)) {
                    // search in ctags
                    base::string hash = getItemId(rowNumber);
                    auto t = customTags::getCustomTag(hash, base::fromStr<int>(c));
                    if (!t.isNil()) {
                        g.drawText(t.getStr(), 5, 0, width, height, Justification::centredLeft, true);
                    }
                }
            }
        }

        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRect(width - 1, 0, 1, height);
    }
};
