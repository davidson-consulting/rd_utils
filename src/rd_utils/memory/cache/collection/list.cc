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

  void ArrayListBase::send (net::TcpStream & stream, uint32_t bufferSize) {
    uint32_t nbInBuffer = bufferSize / this-> _innerSize;
    uint8_t * buffer = new uint8_t [nbInBuffer * this-> _innerSize];
    stream.sendU32 (this-> _size);
    stream.sendU32 (this-> _innerSize);

    for (auto bl : this-> _metadata) {
      AllocatedSegment seg = {.blockAddr = bl, .offset = ALLOC_HEAD_SIZE};
      this-> sendBlock (stream, seg, this-> _allocable, buffer, nbInBuffer);
    }
  }

  void ArrayListBase::sendBlock (net::TcpStream & stream, AllocatedSegment seg, uint32_t nbElements, uint8_t * buffer, uint32_t nbInBuffer) {
    for (uint32_t i = 0 ; i < nbElements ;) {
      auto nb = (nbElements - i) > nbInBuffer ? nbInBuffer : (nbElements - i);
      Allocator::instance ().read (seg, buffer,  i * this-> _innerSize, nb * this-> _innerSize);
      stream.sendRaw (buffer, nb * this-> _innerSize);

      i += nb;
    }
  }


  void ArrayListBase::grow () {
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
