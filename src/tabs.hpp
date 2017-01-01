#pragma once
#include "includes.hpp"
#include "playlist.hpp"

class tabs : public TabbedComponent {
public:
    tabs(TabbedButtonBar::Orientation o) : TabbedComponent(o) {}

    int selectTabByName(const base::string &name) {
        StringArray captions = getTabNames();
        auto index = std::find(captions.begin(), captions.end(), name);
        if (index != captions.end()) {
            int tabIndex = (int)std::distance(captions.begin(), index);
            setCurrentTabIndex(tabIndex);
            return tabIndex;
        }
        return -1;
    }

    void removeTabByName(const base::string &name) {
        StringArray captions = getTabNames();
        auto index = std::find(captions.begin(), captions.end(), name);
        if (index != captions.end()) {
            int tabIndex = (int)std::distance(captions.begin(), index);
            removeTabByIndex(tabIndex);
        }
    }

    void removeTabByIndex(int index) {
        if (getCurrentTabIndex() == index)
            setCurrentTabIndex((index + 1) % getNumTabs());
        removeTab(index);
    }
};
