#pragma once
#include "includes.hpp"

class slider : public Slider {
    bool dragging;

public:
    slider(SliderStyle s, TextEntryBoxPosition eb) : Slider(s, eb), dragging(false) {}
    ~slider() {}

    bool isDragging() const { return dragging; }
    void startedDragging() override { dragging = true; }
    void stoppedDragging() override { dragging = false; }
};
