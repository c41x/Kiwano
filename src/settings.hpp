#pragma once
#include "includes.hpp"

base::cell_t settings_load(base::lisp &, base::cell_t, base::cells_t &);
base::cell_t settings_save(base::lisp &, base::cell_t, base::cells_t &);
base::cell_t settings_get(base::lisp &, base::cell_t, base::cells_t &);
base::cell_t settings_set(base::lisp &, base::cell_t, base::cells_t &);
