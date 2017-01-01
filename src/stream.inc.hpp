#pragma once
#include "includes.hpp"

namespace granite { namespace base {

// reader / writer for: std::map<base::string, std::vector<base::cell>>
typedef std::map<base::string, std::vector<base::cell>> cellMap;

template<> inline size_t stream::read(cellMap &out) {
    uint32 mapSize;
    uint32 readed = 0;
    if ((readed = read(mapSize)) == 0)
        return readed;
    for (uint32 i = 0; i < mapSize; ++i) {
        base::string key;
        auto r = read(key);
        if (r == 0)
            return readed;
        readed += r;
        auto &c = out[key];
        r = read(c);
        if (r == 0)
            return readed;
        readed += r;
    }
    return readed;
}

template<> inline void stream::write(const cellMap &in) {
    write((uint32)in.size());
    for (const auto &e : in) {
        write(e.first);
        write(e.second);
    }
}

}}
