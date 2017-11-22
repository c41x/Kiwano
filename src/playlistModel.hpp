#pragma once
#include "includes.hpp"
#include "seekRange.hpp"
#include "supportedFormats.hpp"
#include "utils.hpp"
#include "graphics.hpp"

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
    std::vector<uint32> tyear, ttrack, tlength;
    std::vector<uint32> paths_i, talbum_i, tartist_i, ttitle_i;
    std::vector<base::string> columns;
    std::vector<base::string> columnsGroup;

    // filtering
    base::string filterQuery;
    bool filterEnabled;

    base::lisp &gl;
    base::string playlistId;

    // colors
    Colour clBackground;
    Colour clBackgroundSelected;
    Colour clBackgroundFilter;
    Colour clBackgroundFilterSelected;
    Colour clBackgroundGroup;
    Colour clText;
    Colour clTextSelected;
    Colour clTextGroup;

    playlistModel(playlistModel &r, const base::string &filter, base::lisp &_gl, const base::string &_playlistId) :
            filterEnabled(false),
            gl(_gl),
            playlistId(_playlistId),
            clBackground(Colour((uint8)33, 33, 33, (uint8)255)),
            clBackgroundSelected(Colour((uint8)66, 66, 66, (uint8)255)),
            clBackgroundFilter(Colour((uint8)255, 0, 0, (uint8)255)),
            clBackgroundFilterSelected(Colour((uint8)255, 255, 0, (uint8)255)),
            clBackgroundGroup(Colour((uint8)11, 11, 11, (uint8)255)),
            clText(Colour((uint8)170, 170, 170, (uint8)255)),
            clTextGroup(Colour((uint8)170, 170, 170, (uint8)255)) {
        // reset
        init();

        // fill (filter)
        const int items = r.getItemsCount();
        for (int i = 0; i < items; ++i) {
            if (r.isTrack(i)) {
                if (r.filterMatch(i, filter)) {
                    paths.append(r.getItemPath(i));
                    talbum.append(r.getItemAlbum(i));
                    tartist.append(r.getItemArtist(i));
                    ttitle.append(r.getItemTitle(i));
                    tyear.push_back(r.getItemYear(i));
                    tlength.push_back(r.getItemLength(i));
                    tseek.push_back(r.getItemSeek(i));
                    ttrack.push_back(r.getItemTrack(i));
                    paths_i.push_back(paths.size());
                    talbum_i.push_back(talbum.size());
                    tartist_i.push_back(tartist.size());
                    ttitle_i.push_back(ttitle.size());
                }
            }
        }
    }

    playlistModel(base::lisp &_gl, const base::string &_playlistId) :
            filterEnabled(false),
            gl(_gl),
            playlistId(_playlistId),
            clBackground(Colour((uint8)33, 33, 33, (uint8)255)),
            clBackgroundSelected(Colour((uint8)66, 66, 66, (uint8)255)),
            clBackgroundFilter(Colour((uint8)255, 0, 0, (uint8)255)),
            clBackgroundFilterSelected(Colour((uint8)255, 255, 0, (uint8)255)),
            clBackgroundGroup(Colour((uint8)11, 11, 11, (uint8)255)),
            clText(Colour((uint8)170, 170, 170, (uint8)255)),
            clTextGroup(Colour((uint8)170, 170, 170, (uint8)255)) {}
    ~playlistModel() {}

    void init() {
        paths_i.clear(); // this one first (getNumRows)
        paths_i.push_back(0);
        paths.clear();
        talbum.clear();
        tartist.clear();
        tyear.clear();
        tlength.clear();
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

    struct itemInfo {
        base::string album, artist, title, path;
        seekRange seek;
        uint32 year, track, length;
    };

    void addItemGroup(const std::vector<String> &group) {
        if (group.size() == 0)
            return;

        std::vector<itemInfo> groupInfo;

        // cues first
        std::vector<size_t> addedByCue;
        for (const auto &f : group) {
            if (f.endsWith(".cue")) {
                addItem(f, groupInfo);
            }
        }

        // then separate files, if not added by cue already
        for (const auto &e : group) {
            addItem(e, groupInfo);
        }

        if (groupInfo.size() > 0) {
            // sort by track
            std::sort(std::begin(groupInfo), std::end(groupInfo),
                      [](const itemInfo &a, const itemInfo &b) {
                          return a.track < b.track;
                      });

            // initialize group, first determine group name, then add dummy item mark
            base::string *albumName = &groupInfo[0].album;

            // check if all items have the same album
            for (const auto &e : groupInfo) {
                if (*albumName != e.album) {
                    albumName = nullptr;
                    break;
                }
            }

            // fallback name (directory name)
            String folderName;
            if (albumName == nullptr) {
                folderName = group[0].upToLastOccurrenceOf("/", false, false).fromLastOccurrenceOf("/", false, false);
            }

            // adding dummy item (group mark)
            paths_i.push_back(paths.size());
            if (albumName == nullptr)
                talbum.append(folderName.toStdString());
            else talbum.append(*albumName);
            talbum_i.push_back(talbum.size());
            tyear.push_back(0);
            tlength.push_back(0);
            tseek.push_back(seekRange());
            ttrack.push_back(0);
            tartist_i.push_back(tartist.size());
            ttitle_i.push_back(ttitle.size());

            // add results to database
            for (const auto &e : groupInfo) {
                paths.append(e.path);
                paths_i.push_back(paths.size());
                talbum.append(e.album);
                tartist.append(e.artist);
                ttitle.append(e.title);
                tyear.push_back(e.year);
                tlength.push_back(e.length);
                tseek.push_back(e.seek);
                ttrack.push_back(e.track);
                talbum_i.push_back(talbum.size());
                tartist_i.push_back(tartist.size());
                ttitle_i.push_back(ttitle.size());
            }
        }
    }

    void addItem(const String &path, std::vector<itemInfo> &groupInfo) {
        auto processCue = [&groupInfo](const char *data,
                                       const base::string &basePath,
                                       const base::string &gpath) {{
                Cd *cd = cue_parse_string(data);
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
                                //int trackIndex = -1;
                                int start = 0;
                                int length = 0;
                                const char *date = nullptr;

                                if (cdtext != nullptr) {
                                    artist = cdtext_get(PTI_PERFORMER, cdtext);
                                    title = cdtext_get(PTI_TITLE, cdtext);
                                    //trackIndex = track_get_index(track, i);
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

                                // search for valid path. often CUEs have invalid file extension (output from ExactAudioCopy)
                                bool fileFound = true;
                                auto fname = base::strs(basePath, GE_DIR_SEPARATOR, path);
                                if (!base::fs::exists(fname)) {
                                    fileFound = false;
                                    for (const auto &f : supportedFormats::formats) {
                                        fname = base::changeExt(fname, f);
                                        if (base::fs::exists(fname)) {
                                            fileFound = true;
                                            break;
                                        }
                                    }
                                }

                                if (fileFound) {
                                    // trackIndex is in frames (it is *not* track index)
                                    uint32 track = i; //trackIndex < 0 ? i : (trackIndex + 1);
                                    uint32 year;
                                    if (base::strIs<int>(date))
                                        year = base::fromStr<int>(date);
                                    else if (base::strIs<int>(defaultDate))
                                        year = base::fromStr<int>(defaultDate);
                                    else year = 0;

                                    bool foundWithTheSamePath = false;
                                    std::vector<itemInfo>::iterator item = groupInfo.end();
                                    for (size_t j = 0; j < groupInfo.size(); ++j) {
                                        if (fname == groupInfo[j].path) {
                                            foundWithTheSamePath = true;

                                            if (groupInfo[j].seek.start == start &&
                                                groupInfo[j].seek.end == start + length) {
                                                item = groupInfo.begin() + j;
                                                break;
                                            }
                                        }
                                    }

                                    bool foundWithDifferentSeek = item == groupInfo.end();

                                    if (!foundWithTheSamePath || foundWithDifferentSeek) {
                                        itemInfo ii;
                                        ii.path = fname;
                                        ii.album = defaultTitle;
                                        ii.artist = artist;
                                        ii.seek = {start, start + length};
                                        ii.title = title;
                                        ii.track = track;
                                        ii.year = year;
                                        ii.length = (length * 1000) / 75; // frames to milliseconds

                                        // 0 length cue -> last track maybe - try to extract info from file
                                        if (length <= 0) {
                                            TagLib::FileRef file(fname.c_str());
                                            if (!file.isNull() && file.audioProperties()) {
                                                ii.length = file.audioProperties()->length() * 1000 - ((start * 1000) / 75);
                                            }
                                        }

                                        groupInfo.push_back(ii);
                                    }
                                    else {
                                        // try to refine info
                                        itemInfo &ii = *item;

                                        // it is the same item, weird... duplicated CUE? refining info (the more entrophy - the better)
                                        ii.album = longest(ii.album, defaultTitle);
                                        ii.artist = longest(ii.artist, artist);
                                        ii.title = longest(ii.title, title);
                                        ii.year = std::max(ii.year, year);
                                    }
                                }
                            }
                        }
                    }
                }
                logInfo(base::strs("CUE: could not parse / syntax error in: ", gpath));
            }
        };

        base::string gpath = path.toStdString();
        base::string basePath = base::extractFilePath(gpath);

        if (base::extractExt(gpath) == "cue") {
            base::stream s = base::fs::load(gpath);
            if (s.size() > 0) {
                processCue((const char*)s.data(), basePath, gpath);
            }
            else {
                logInfo(base::strs("could not load CUE - empty file: ", gpath));
            }
        }
        else {
            base::string album, artist, title;
            uint32 track, year, length;

            TagLib::FileRef file(path.toRawUTF8());

            // search for embeeded CUE
            bool cueLoaded = false;
            TagLib::PropertyMap tags = file.file()->properties();
            for (TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
                for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
                    if (base::lowerCase(i->first.to8Bit()) == "cuesheet") {
                        // processCue(j->toCString(), basePath, gpath);
                        //cueLoaded = true;
                    }
                }
            }

            if (!cueLoaded) {
                if (!file.isNull() && file.tag()) {
                    album = file.tag()->album().toCString();
                    artist = file.tag()->artist().toCString();
                    title = file.tag()->title().toCString();
                    year = file.tag()->year();
                    track = file.tag()->track();
                }
                else {
                    album = "";
                    artist = "";
                    title = "";
                    year = 0;
                    track = 0;
                }

                if (!file.isNull() && file.audioProperties()) {
                    // TODO: lengthInMilliseconds in future taglib API
                    length = file.audioProperties()->length() * 1000;
                }
                else {
                    length = 0;
                }

                auto item = std::find_if(std::begin(groupInfo), std::end(groupInfo),
                                         [&gpath](const itemInfo &it) {
                                             return gpath == it.path;
                                         });

                if (item != groupInfo.end()) {
                    // do not refine info
                }
                else {
                    itemInfo ii;
                    ii.path = gpath;
                    ii.album = album;
                    ii.artist = artist;
                    ii.seek = seekRange();
                    ii.title = title;
                    ii.track = track;
                    ii.year = year;
                    ii.length = length;
                    groupInfo.push_back(ii);
                }
            }
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
        return getItemPathR(index).str();
    }

    base::stringRange getItemPathR(size_t index) const {
        return base::stringRange(paths.begin() + paths_i[index],
                                 paths.begin() + paths_i[index + 1]);
    }

    bool isTrack(size_t index) const {
        return getItemPathR(index).count() != 0;
    }

    uint getItemTrack(size_t index) const {
        return ttrack[index];
    }

    uint getItemYear(size_t index) const {
        return tyear[index];
    }

    uint32 getItemLength(size_t index) const {
        return tlength[index];
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
        return paths_i.size() - 1;
    }

    int getNumRows() override {
        return getItemsCount();
    }

    bool setColor(const base::string &id, Colour color) {
        if (id == "bg") {
            clBackground = color;
            return true;
        }
        else if (id == "bg-selected") {
            clBackgroundSelected = color;
            return true;
        }
        else if (id == "bg-filter") {
            clBackgroundFilter = color;
            return true;
        }
        else if (id == "bg-filter-selected") {
            clBackgroundFilterSelected = color;
            return true;
        }
        else if (id == "bg-group") {
            clBackgroundGroup = color;
            return true;
        }
        else if (id == "txt") {
            clText = color;
            return true;
        }
        else if (id == "txt-group") {
            clTextGroup = color;
            return true;
        }
        return false;
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override {
        if (filterEnabled && filterMatch(rowNumber, filterQuery)) {
            if (rowIsSelected)
                g.fillAll(clBackgroundFilterSelected);
            else g.fillAll(clBackgroundFilter);
        }
        else if (rowIsSelected) {
            g.fillAll(clBackgroundSelected);
        }
        else {
            if (rowNumber < getNumRows() && !isTrack(rowNumber)) {
                g.fillAll(clBackgroundGroup);
            }
            else {
                g.fillAll(clBackground);
            }
        }
    }

    base::string formatTime(int milliseconds) {
        int secondsTotal = milliseconds / 1000;
        int seconds = secondsTotal % 60;
        int minutes = (secondsTotal / 60) % 60;
        int hours = secondsTotal / 60 / 60;

        base::string ret;
        ret.reserve(15);
        if (hours > 0) {
            ret += base::toStr(hours);
            if (minutes < 10)
                ret += ":0";
            else ret += ":";
        }

        ret += base::toStr(minutes);
        if (seconds < 10)
            ret += ":0";
        else ret += ":";
        ret += base::toStr(seconds);

        return ret;
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell(Graphics& g, int rowNumber, int columnId,
                   int width, int height, bool /*rowIsSelected*/) override {

        if (columnId < (int)columns.size() && rowNumber < getNumRows()) {
            auto &c = columns[columnId];
            auto &cg = columnsGroup[columnId];

            // group begin
            if (!isTrack(rowNumber)) {
                g.setFont(juce::Font("Ubuntu Condensed", height, juce::Font::plain));

                if (cg == "album") {
                    g.setColour(clTextGroup);
                    g.drawText(getItemAlbum(rowNumber), 0, 0, width, height, Justification::centred, true);
                }
                else if (!cg.empty()) {
                    graphics::g = &g;
                    gl.eval(base::strs("(", cg, " '", playlistId, " ", rowNumber, " ", width, " ", height, ")"));
                    graphics::g = nullptr;
                }
                return;
            }
            else {
                g.setColour(clText);
                g.setFont(juce::Font("Ubuntu Condensed", height, juce::Font::plain));

                // item
                if (c == "track")
                    g.drawText(base::toStr(getItemTrack(rowNumber)), 5, 0, width, height, Justification::centredLeft, true);
                else if (c == "album")
                    g.drawText(getItemAlbum(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
                else if (c == "artist")
                    g.drawText(getItemArtist(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
                else if (c == "title")
                    g.drawText(getItemTitle(rowNumber), 5, 0, width, height, Justification::centredLeft, true);
                else if (c == "year")
                    g.drawText(base::toStr(getItemYear(rowNumber)), 5, 0, width, height, Justification::centredLeft, true);
                else if (c == "length")
                    g.drawText(formatTime(getItemLength(rowNumber)), 0, 0, width - 5, height, Justification::centredRight, true);
                else {
                    // bind graphics
                    graphics::g = &g;

                    // custom code
                    gl.eval(base::strs("(", c, " '", playlistId, " ", rowNumber, " ", width, " ", height, ")"));

                    graphics::g = nullptr;
                }
            }
        }

        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRect(width - 1, 0, 1, height);
    }
};
