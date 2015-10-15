#pragma once
#include "includes.hpp"

namespace playback {
AudioDeviceManager dm;
AudioFormatManager fm;
AudioSourcePlayer asp;
AudioTransportSource ts;
ScopedPointer<AudioFormatReaderSource> frs;
TimeSliceThread thread("audio playback");

void init() {
	fm.registerBasicFormats();
	thread.startThread(3);
	dm.addAudioCallback(&asp);
	asp.setSource(&ts);
}

void shutdown() {
	ts.setSource(nullptr);
	asp.setSource(nullptr);
	dm.removeAudioCallback(&asp);
}

//- LISP API -
// playback-set-file (string)fileName -> t/nil
base::cell_t set_file(base::lisp &gl, base::cell_t c, base::cells_t &) {
	const auto &fname = c + 1;
	ts.stop();
	ts.setSource(nullptr);
	frs = nullptr;
	AudioFormatReader *r = fm.createReaderFor(File(fname->s));
	if (r) {
		frs = new AudioFormatReaderSource(r, true);
		ts.setSource(frs, 32768, &thread, r->sampleRate);
		gl.signalError(base::strs("file not found or file format not supported: ", fname->s));
		return gl.t();
	}
	return gl.nil();
}

// playback-unload-file
base::cell_t unload_file(base::lisp &gl, base::cell_t, base::cells_t &) {
	ts.stop();
	ts.setSource(nullptr);
	frs = nullptr;
	return gl.nil();
}

// playback-start
base::cell_t start(base::lisp &gl, base::cell_t, base::cells_t &) {
	ts.start();
	return gl.nil();
}

// playback-stop
base::cell_t stop(base::lisp &gl, base::cell_t, base::cells_t &) {
	ts.stop();
	return gl.nil();
}

// playback-seek (float)posSeconds
base::cell_t seek(base::lisp &gl, base::cell_t c, base::cells_t &) {
	const auto &pos = c + 1;
	ts.setPosition((double)pos->f);
	return gl.nil();
}

// playback-length -> (float)/total time in seconds/
base::cell_t length(base::lisp &gl, base::cell_t, base::cells_t &ret) {
	ret.push_back(base::cell(float(ts.getLengthInSeconds())));
	return ret.end() - 1;
}

// playback-get-pos -> (float)/playback position in seconds/
base::cell_t get_pos(base::lisp &gl, base::cell_t, base::cells_t &ret) {
	ret.push_back(base::cell(float(ts.getCurrentPosition())));
	return ret.end() - 1;
}

// playback-is-playing -> t/nil
base::cell_t is_playing(base::lisp &gl, base::cell_t, base::cells_t &) {
	if (ts.isPlaying())
		return gl.t();
	return gl.nil();
}

}

// TODO: validation
// TODO: cue files
