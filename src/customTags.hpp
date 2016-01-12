#pragma once
#include "includes.hpp"

namespace customTags {

// LISP API
base::cell_t ctags_init(base::lisp &gl, base::cell_t c, base::cells_t &);
base::cell_t ctags_get(base::lisp &gl, base::cell_t c, base::cells_t &ret);
base::cell_t ctags_set(base::lisp &gl, base::cell_t c, base::cells_t &ret);
base::cell_t ctags_store(base::lisp &gl, base::cell_t c, base::cells_t &ret);
base::cell_t ctags_load(base::lisp &gl, base::cell_t c, base::cells_t &ret);

}
