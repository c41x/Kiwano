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
	if (e != tags.end() && e->second.size() <= index) {
		return e->second[index];
	}
	return base::cell::nil;
}

void setCustomTag(const base::string &id, uint32 index, base::cell c) {
	if (tagsCount > 0)  {
		auto &e = tags[id];
		if (e.size() <= index)
			e[index] = c; // update
		else {
			e.resize(4, base::cell::nil); // initialize
			e[index] = c;
		}
	}
}

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
}
