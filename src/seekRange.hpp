#pragma once
#include "includes.hpp"

// seek in frames (1 = 75 frames per second)
struct seekRange {
    int32 start;
    int32 end;
    seekRange() : start(0), end(0) {}
    seekRange(int32 _start, int32 _end) : start(_start), end(_end) {}
    bool empty() const { return start == 0 && end == 0; }
    base::string toString() const { return base::strs(start, ':', end); }
};

namespace granite { namespace base {
// overloads for writing seekRange
template <> inline size_t stream::read(seekRange &out) {
    return read<int32>(out.start) + read<int32>(out.end);
}

template <> inline void stream::write(const seekRange &in) {
    write<int32>(in.start);
    write<int32>(in.end);
}
}}
