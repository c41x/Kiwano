#pragma once
#include "includes.hpp"
#include "playlist.hpp"

class tabs : public TabbedComponent {
public:
    tabs(TabbedButtonBar::Orientation o) : TabbedComponent(o) {}
};
