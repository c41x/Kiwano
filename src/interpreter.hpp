#pragma once
#include "includes.hpp"

class interpreter : public Component,
                    public TextEditor::Listener {
    base::lisp &gl;
    TextEditor te;

public:
    interpreter(base::lisp &glisp) : gl(glisp) {
        te.setMultiLine(true);
        te.setReturnKeyStartsNewLine(false);
        te.setTabKeyUsedAsCharacter(false);
        te.setReadOnly(false);
        te.setCaretVisible(true);
        te.addListener(this);
        addAndMakeVisible(te);
    }
    ~interpreter() {}

    void resized() override {
        te.setBounds(getLocalBounds());
    }

    void textEditorTextChanged(TextEditor&) override {}
    void textEditorEscapeKeyPressed(TextEditor&) override {}
    void textEditorFocusLost(TextEditor&) override {}

    void textEditorReturnKeyPressed(TextEditor&) override {
        // get line
        te.moveCaretToStartOfLine(false);
        int begin = te.getCaretPosition();
        te.moveCaretToEndOfLine(false);
        String t = te.getTextInRange(Range<int>(begin, te.getCaretPosition()));

        // execute code and print result
        std::string r = gl.eval(t.toStdString());
        te.insertTextAtCaret("\n > ");
        te.insertTextAtCaret(r);
        te.insertTextAtCaret("\n");
    }
};
