#pragma once
#include "includes.hpp"

class layout : public Component {
	StretchableLayoutManager l;
	std::vector<std::unique_ptr<Component>> components;
	std::vector<Component*> lc;
	bool horizontal;

public:
	layout(bool _horizontal) : horizontal(_horizontal){
		setOpaque(true);
	}

	void paint(Graphics &g) override {
		g.setColour(Colour::greyLevel(0.2f));
		g.fillAll();
	}

	void resized() override {
		juce::Rectangle<int> r(getLocalBounds());
		l.layOutComponents(lc.data(), lc.size(), r.getX(), r.getY(), r.getWidth(), r.getHeight(), !horizontal, true);
	}

	void addSplitter() {
		if (lc.size() > 0) {
			l.setItemLayout(lc.size(), 5, 5, 5);
			components.push_back(std::make_unique<StretchableLayoutResizerBar>(&l, lc.size(), horizontal));
			lc.push_back(components.back().get());
			addAndMakeVisible(components.back().get());
		}
	}

	void addComponent(Component *c, double minimum, double maximum, double preferred) {
		l.setItemLayout(lc.size(), minimum, maximum, preferred);
		lc.push_back(c);
		addAndMakeVisible(c);
	}
};
