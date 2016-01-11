#pragma once
#include "includes.hpp"

namespace customTags {

void initTags(uint32 count);
base::cell getCustomTag(const base::string &id, uint32 index);
void setCustomTag(const base::string &id, uint32 index, base::cell c);
bool storeTags(const base::string &f);
bool loadTags(const base::string &f);

}
