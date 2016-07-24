#pragma once
#include "includes.hpp"
#include <regex>

namespace playback {
AudioDeviceManager dm;
AudioFormatManager fm;
AudioSourcePlayer asp;
AudioTransportSource ts;
ScopedPointer<AudioFormatReaderSource> frs;
TimeSliceThread thread("audio playback");

class playbackListener : public ChangeListener {
	base::lisp &gl;
public:
	base::string functionId;
	bool enabled;
	playbackListener(base::lisp &gli) : gl(gli), enabled(false) {}
	void changeListenerCallback(ChangeBroadcaster *source) override {
		if (source == &ts && enabled) {
			gl.eval(base::strs("(", functionId, ")"));
		}
	}
};

playbackListener *pl = nullptr;

void init(base::lisp &gl) {
	fm.registerBasicFormats();
	thread.startThread(3);
	dm.addAudioCallback(&asp);
	asp.setSource(&ts);
	pl = new playbackListener(gl);
	ts.addChangeListener(pl);
}

void shutdown() {
	ts.removeAllChangeListeners();
	ts.setSource(nullptr);
	asp.setSource(nullptr);
	dm.removeAudioCallback(&asp);
	delete pl;
	pl = nullptr;
}

//- LISP API -
// playback-set-file (string)fileName -> t/nil
base::cell_t set_file(base::lisp &gl, base::cell_t c, base::cells_t &) {
	if (base::lisp::validate(c, base::cell::list(1), base::cell::typeString)) {
		const auto &fname = c + 1;

		// stop current playback
		ts.stop();
		ts.setSource(nullptr);
		frs = nullptr;

		AudioFormatReader *r;

		// ectract CUE information (if any)
		std::regex cue("^(.*):(\\d+):(\\d+)$");
		std::smatch result;
		std::regex_search(fname->s, result, cue);
		if (result.size() == 4) {
			// is cue
			int32 start = base::fromStr<int32>(result[2].str());
			int32 end = base::fromStr<int32>(result[3].str());
			AudioFormatReader *tr = fm.createReaderFor(File(result[1].str()));
			r = new AudioSubsectionReader(tr, start, end - start, true);
		}
		else {
			// regular file
			r = fm.createReaderFor(File(fname->s));
		}

		if (r) {
			frs = new AudioFormatReaderSource(r, true);
			ts.setSource(frs, 32768, &thread, r->sampleRate);
			return gl.t();
		}
		gl.signalError(base::strs("file not found or file format not supported: ", fname->s));
		return gl.nil();
	}
	gl.signalError("playback-set-file: invalid arguments, expected (string)");
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
	if (base::lisp::validate(c, base::cell::list(1), base::cell::typeFloat)) {
		const auto &pos = c + 1;
		ts.setPosition((double)pos->f);
		return gl.nil();
	}
	gl.signalError("playback-seek: invalid arguments, expected (float)");
	return gl.nil();
}

// playback-length -> (float)/total time in seconds/
base::cell_t length(base::lisp &gl, base::cell_t, base::cells_t &ret) {
	ret.push_back(base::cell(float(ts.getLengthInSeconds())));
	return ret.end();
}

// playback-get-pos -> (float)/playback position in seconds/
base::cell_t get_pos(base::lisp &gl, base::cell_t, base::cells_t &ret) {
	ret.push_back(base::cell(float(ts.getCurrentPosition())));
	return ret.end();
}

// playback-is-playing -> t/nil
base::cell_t is_playing(base::lisp &gl, base::cell_t, base::cells_t &) {
	if (ts.isPlaying())
		return gl.t();
	return gl.nil();
}

// playback-finished -> t/nil
base::cell_t finished_playing(base::lisp &gl, base::cell_t, base::cells_t &) {
	if (ts.hasStreamFinished())
		return gl.t();
	return gl.nil();
}

// (bind-playback (id)callback)
base::cell_t bind_playback(base::lisp &gl, base::cell_t c, base::cells_t &) {
	if (pl) {
		if (base::lisp::validate(c, base::cell::list(1), base::cell::typeIdentifier)) {
			const auto &fx = c + 1;
			pl->functionId = fx->s;
			pl->enabled = true;
			return gl.t();
		}
		pl->enabled = false;
		return gl.nil();
	}
	gl.signalError("bind-playback: invalid arguments, expected (id)");
	return gl.nil();
}

// (playback-gain (float|optional)gain) -> t/nil | gain
base::cell_t gain(base::lisp &gl, base::cell_t c, base::cells_t &ret) {
	if (pl) {
		if (base::lisp::validate(c, base::cell::listRange(1), base::cell::typeFloat)) {
			// setter
			ts.setGain((c + 1)->f);
			return gl.t();
		}
		else if (base::lisp::validate(c, base::cell::list(0))) {
			// getter
			ret.push_back(base::cell(ts.getGain()));
			return ret.end();
		}
		gl.signalError("playback-gain: invalid arguments, expected ((optional) float)");
	}
	return gl.nil();
}

// (unbind-playback)
base::cell_t unbind_playback(base::lisp &gl, base::cell_t, base::cells_t &) {
	if (pl) {
		pl->enabled = false;
		return gl.t();
	}
	return gl.nil();
}

}

// TODO: cue files
// TODO: save playback settings
