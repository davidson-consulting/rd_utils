#include "list.hh"

namespace rd_utils::memory::cache::collection {

  ArrayListBase::ArrayListBase () :
    _metadata ()
    , _size (0)
    , _innerSize (0)
    , _allocable (0)
  {}

  ArrayListBase::ArrayListBase (uint32_t nbBlocks, uint32_t innerSize) :
    _metadata ()
    , _size (0)
    , _innerSize (innerSize)
    , _allocable (0)
  {
    while (nbBlocks > this-> _metadata.size ()) this-> grow ();
    auto all = Allocator::instance ().getMaxAllocable () - sizeof (uint32_t);
    this-> _allocable = (all - (all % innerSize)) / innerSize;
  }

  uint32_t ArrayListBase::len () const {
    return this-> _size;
  }

  uint32_t ArrayListBase::nbBlocks () const {
    return this-> _metadata.size ();
  }

  ArrayListBase::~ArrayListBase () {
    this-> dispose ();
  }

  void ArrayListBase::grow () {
    std::cout << "New block" << std::endl;
    AllocatedSegment seg;
    Allocator::instance ().allocate (this-> _allocable, seg, true, false);
    this-> _metadata.push_back (seg.blockAddr);
  }

  void ArrayListBase::dispose () {
    for (auto & it : this-> _metadata) {
      Allocator::instance ().freeFast (it);
    }

    this-> _metadata.clear ();
    this-> _size = 0;
  }

}
