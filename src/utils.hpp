#pragma once

#include "includes.hpp"

base::string longest(const base::string &a, const base::string &b) {
    if (a.size() > b.size())
        return a;
    return b;
}
