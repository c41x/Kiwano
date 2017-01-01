#include "settings.hpp"
#include "stream.inc.hpp"

namespace {
std::map<base::string, std::vector<base::cell>> cellsMap;
}

base::cell_t settings_load(base::lisp &gl, base::cell_t c, base::cells_t &) {
    if (base::lisp::validate(c, base::cell::list(1), base::cell::typeString)) {
        const auto &fileName = c + 1;
        base::stream s = base::fs::load(fileName->s);
        if (s.read(cellsMap) == 0)
            return gl.nil();
        return gl.t();
    }
    gl.signalError("settings-save: invalid arguments, expected (string)");
    return gl.nil();
}

base::cell_t settings_save(base::lisp &gl, base::cell_t c, base::cells_t &) {
    if (base::lisp::validate(c, base::cell::list(1), base::cell::typeString)) {
        const auto &fileName = c + 1;
        base::stream s;
        s.write(cellsMap);
        if (base::fs::store(fileName->s, s))
            return gl.t();
        return gl.nil();
    }
    gl.signalError("settings-load: invalid arguments, expected (string)");
    return gl.nil();
}

base::cell_t settings_get(base::lisp &gl, base::cell_t c, base::cells_t &ret) {
    if (base::lisp::validate(c, base::cell::list(1), base::cell::typeString)) {
        const auto &key = c + 1;
        auto e = cellsMap.find(key->s);
        if (e != cellsMap.end()) {
            ret.insert(ret.end(), e->second.begin(), e->second.end());
            return ret.end();
        }
        return gl.nil();
    }
    gl.signalError("settings-get: invalid arguments, expected (string)");
    return gl.nil();
}

base::cell_t settings_set(base::lisp &gl, base::cell_t c, base::cells_t &) {
    if (base::lisp::validate(c, base::cell::list(2), base::cell::typeString /* any */)) {
        const auto &key = c + 1;
        const auto &value = c + 2;
        auto &e = cellsMap[key->s];
        granite::int32 size = base::lisp::size(value);
        e.assign(value, value + size);
        return gl.t();
    }
    gl.signalError("settings-set: invalid arguments, expected (string string)");
    return gl.nil();
}
