#include "customTags.hpp"

namespace customTags {

// persistent user defined tags map
std::map<base::string, std::vector<base::cell>> tags;
uint32 tagsCount = 0;

void initTags(uint32 count) {
	tagsCount = count;
	// TODO: resizing?
}

base::cell getCustomTag(const base::string &id, uint32 index) {
	auto e = tags.find(id);
	if (e != tags.end() && index < e->second.size()) {
		return e->second[index];
	}
	return base::cell::nil;
}

void setCustomTag(const base::string &id, uint32 index, base::cell c) {
	if (tagsCount > 0)  {
		auto &e = tags[id];
		if (e.size() < index)
			e[index] = c; // update
		else {
			e.resize(tagsCount, base::cell::nil); // initialize
			e[index] = c;
		}
	}
}

// TODO: fix this!
bool storeTags(const base::string &f) {
	base::stream s;
	s.write((uint32)tags.size());
	for (const auto &e : tags) {
		s.write(e.first);
		// TODO: write returns / estimate size / write documentation on writing vectors?
		s.write(sizeof(uint32) + sizeof(base::cell) * e.second.size());
		s.write(e.second);
	}
	return base::fs::store(f, s);
}

bool loadTags(const base::string &f) {
	tags.clear();
	base::stream s = base::fs::load(f);
	uint32 mapSize;
	if (s.read(mapSize) == 0)
		return false;
	for (uint32 i = 0; i < mapSize; ++i) {
		base::string key;
		if (s.read(key) == 0)
			return false;
		auto &c = tags[key];
		uint32 cellsSize;
		if (s.read(cellsSize) == 0)
			return false;
		if (s.read(c) != cellsSize)
			return false;
	}
	return true;
}

//- LISP API
// (ctags-init (int|items-count)) -> nil/t
base::cell_t ctags_init(base::lisp &gl, base::cell_t c, base::cells_t &) {
	if (base::lisp::validate(c, base::cell::list(1), base::cell::typeInt)) {
		const auto &num = c + 1;
		initTags(num->i);
		return gl.t();
	}
	gl.signalError("ctags-init: invalid arguments, expected (int)");
	return gl.nil();
}

// (ctags-get (string|id) (int|items-count)) -> any/nil
base::cell_t ctags_get(base::lisp &gl, base::cell_t c, base::cells_t &ret) {
	if (base::lisp::validate(c, base::cell::list(2), base::cell::typeString, base::cell::typeInt)) {
		const auto &key = c + 1;
		const auto &num = c + 2;
		ret.push_back(getCustomTag(key->s, num->i));
		return ret.end();
	}
	gl.signalError("ctags-get: invalid arguments, expected (string int)");
	return gl.nil();
}

// (ctags-set (string|id) (int|items-count) any) -> nil/t
base::cell_t ctags_set(base::lisp &gl, base::cell_t c, base::cells_t &ret) {
	if (base::lisp::validate(c, base::cell::list(3), base::cell::typeString, base::cell::typeInt)) { // TODO: validation
		const auto &key = c + 1;
		const auto &num = c + 2;
		const auto &value = c + 3;
		setCustomTag(key->s, num->i, *value);
		return gl.t();
	}
	gl.signalError("ctags-set: invalid arguments, expected (string int any)");
	return gl.nil();
}

// (ctags-store (string|file-name)) -> nil/t
base::cell_t ctags_store(base::lisp &gl, base::cell_t c, base::cells_t &ret) {
	if (base::lisp::validate(c, base::cell::list(1), base::cell::typeString)) {
		const auto &fileName = c + 1;
		if (storeTags(fileName->s))
			return gl.t();
		return gl.nil();
	}
	gl.signalError("ctags-save: invalid arguments, expected (string)");
	return gl.nil();
}

// (ctags-load (string|file-name)) -> nil/t
base::cell_t ctags_load(base::lisp &gl, base::cell_t c, base::cells_t &ret) {
	if (base::lisp::validate(c, base::cell::list(1), base::cell::typeString)) {
		const auto &fileName = c + 1;
		if (loadTags(fileName->s))
			return gl.t();
		return gl.nil();
	}
	gl.signalError("ctags-load: invalid arguments, expected (string)");
	return gl.nil();
}

}
