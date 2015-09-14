#pragma once
#include "includes.hpp"
#include "playlist.hpp"

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
