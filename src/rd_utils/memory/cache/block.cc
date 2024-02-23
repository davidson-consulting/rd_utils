#include "block.hh"

namespace rd_utils::memory::cache {

  Block::Block (uint64_t addr, uint64_t total_size) :
    _addr (addr)
    , _total_size (total_size)
    , _segments ()
  {
    this-> _segments.push_back ({.offset = 0, .size = total_size});
  }

  bool Block::allocate (uint64_t size, MemorySegment & seg) {
    if (size <= 0 || size > this-> _total_size) {
      seg = {.offset = 0, .size = 0};
      return false;
    }

    for (auto it = this-> _segments.begin () ; it != this-> _segments.end () ; it++) {
      if (it-> size >= size) {
        seg = {.offset = it-> offset, .size = size};
        auto restSize = it-> size - size;
        if (restSize == 0) {
          this-> _segments.erase (it);
          return true;
        } else {
          this-> _segments.insert (it, {.offset = it-> offset + size, .size = restSize});
          this-> _segments.erase (it);
          return true;
        }
      }
    }

    seg = {.offset = 0, .size = 0};
    return false;
  }

  void Block::free (MemorySegment seg) {
    for (auto it = this-> _segments.begin () ; it != this-> _segments.end () ; it++) {
      if (it-> offset > seg.offset) {
        this-> _segments.insert (it, seg);
        this-> merge (it--);
        return;
      }
    }

    this-> _segments.push_back (seg);
    this-> merge (--this-> _segments.end ());
  }


  void Block::merge (std::list<MemorySegment>::iterator it) {
    if (it != this-> _segments.begin ()) {
      auto prev = it;
      prev --;
      auto curr = std::next (prev);

      if (prev-> offset + prev-> size == curr-> offset) {
        prev-> size += curr-> size;
        this-> _segments.erase (curr);
        this-> merge (prev);
        return;
      }
    }

    auto next = std::next (it);
    if (next != this-> _segments.end ()) {
      if (it-> offset + it-> size == next-> offset) {
        next-> offset = it-> offset;
        next-> size = it-> size + next-> size;
        this-> _segments.erase (it);
      }
    }
  }

  Block::~Block () {}

  std::ostream & operator<< (std::ostream & s, const Block & b) {
    s << "B{";
    int i = 0;
    for (auto & it : b._segments) {
      if (i != 0) s << ", ";
      s << "(" << it.offset << "," << (it.offset + it.size) << ")";
    }
    s << "}";
    return s;
  }

}


std::ostream & operator<< (std::ostream & s, const rd_utils::memory::cache::MemorySegment & seg) {
  s << "(" << seg.offset << "," << (seg.offset + seg.size) << ")";
  return s;
}
