#include "array.hh"


namespace rd_utils::memory::cache::collection {

  CacheArrayBase::CacheArrayBase () :
    _rest ({0, 0})
    , _fstBlockAddr (0)
    , _size (0)
    , _innerSize (0)
    , _nbBlocks (0)
    , _sizePerBlock (0)
    , _sizeDividePerBlock (0)
  {}

  CacheArrayBase::CacheArrayBase (uint32_t size, uint32_t innerSize) {
    this-> allocate (size, innerSize);
  }

  void CacheArrayBase::allocate (uint32_t size, uint32_t innerSize) {
    uint32_t nbBl;
    Allocator::instance ().allocateSegments (innerSize, ((uint64_t) size) * ((uint64_t) innerSize), this-> _rest, this-> _fstBlockAddr, nbBl, this-> _sizePerBlock);
    if (nbBl == 0) {
      this-> _nbBlocks = 0;
      this-> _sizeDividePerBlock = 1;
      this-> _sizePerBlock = 0;
    }
    else {
      this-> _nbBlocks = nbBl;
      this-> _sizeDividePerBlock = this-> _sizePerBlock / innerSize;
    }

    this-> _size = size;
    this-> _innerSize = innerSize;
  }

  void CacheArrayBase::move (CacheArrayBase * other) {
    this-> dispose ();

    this-> _rest = other-> _rest;
    this-> _fstBlockAddr = other-> _fstBlockAddr;
    this-> _nbBlocks = other-> _nbBlocks;
    this-> _size = other-> _size;
    this-> _innerSize = other-> _innerSize;
    this-> _sizePerBlock = other-> _sizePerBlock;
    this-> _sizeDividePerBlock = other-> _sizeDividePerBlock;

    other-> _rest = {0, 0};
    other-> _fstBlockAddr = 0;
    other-> _nbBlocks = 0;
    other-> _size = 0;
    other-> _sizeDividePerBlock = 0;
    other-> _sizePerBlock = 0;
  }

  CacheArrayBase::CacheArrayBase (CacheArrayBase * other) {
    this-> move (other);
  }

  void CacheArrayBase::send (net::TcpStream & stream, uint32_t bufferSize) {
    uint32_t nbInBuffer = bufferSize / this-> _innerSize;
    uint8_t * buffer = new uint8_t [nbInBuffer * this-> _innerSize];
    stream.sendU32 (this-> _size);
    stream.sendU32 (this-> _innerSize);

    for (uint32_t i = 0 ; i < this-> _nbBlocks ; i++) {
      AllocatedSegment seg = {.blockAddr = this-> _fstBlockAddr + i, .offset = ALLOC_HEAD_SIZE};
      this-> sendBlock (stream, seg, this-> _sizeDividePerBlock, buffer, nbInBuffer);
    }

    auto globIndex = this-> _nbBlocks * this-> _sizeDividePerBlock;
    this-> sendBlock (stream, this-> _rest, this-> _size - globIndex, buffer, nbInBuffer);
    delete [] buffer;
  }

  void CacheArrayBase::recv (net::TcpStream & stream, uint32_t bufferSize) {
    uint32_t size = stream.receiveU32 ();
    uint32_t innerSize = stream.receiveU32 ();

    this-> dispose ();
    this-> allocate (size, innerSize);

    uint32_t nbInBuffer = bufferSize / this-> _innerSize;
    uint8_t * buffer = new uint8_t [nbInBuffer * this-> _innerSize];
    for (uint32_t i = 0 ; i < this-> _nbBlocks ; i++) {
      AllocatedSegment seg = {.blockAddr = this-> _fstBlockAddr + i, .offset = ALLOC_HEAD_SIZE};
      this-> recvBlock (stream, seg, this-> _sizeDividePerBlock, buffer, nbInBuffer);
    }

    auto globIndex = this-> _nbBlocks * this-> _sizeDividePerBlock;
    this-> recvBlock (stream, this-> _rest, this-> _size - globIndex, buffer, nbInBuffer);
    delete [] buffer;
  }



  void CacheArrayBase::sendBlock (net::TcpStream & stream, AllocatedSegment seg, uint32_t nbElements, uint8_t * buffer, uint32_t nbInBuffer) {
    for (uint32_t i = 0 ; i < nbElements ;) {
      auto nb = (nbElements - i) > nbInBuffer ? nbInBuffer : (nbElements - i);
      Allocator::instance ().read (seg, buffer,  i * this-> _innerSize, nb * this-> _innerSize);
      stream.send ((char*) buffer, nb * this-> _innerSize);

      i += nb;
    }
  }

  void CacheArrayBase::recvBlock (net::TcpStream & stream, AllocatedSegment seg, uint32_t nbElements, uint8_t * buffer, uint32_t nbInBuffer) {
    for (uint32_t i = 0 ; i < nbElements ; ) {
      auto nb = (nbElements - i) > nbInBuffer ? nbInBuffer : (nbElements - i);
      if (stream.receiveRaw (buffer, nb * this-> _innerSize)) {
        Allocator::instance ().write (seg, buffer, i * this-> _innerSize, nb * this-> _innerSize);
      }

      i += nb;
    }
  }

  void CacheArrayBase::dispose () {
    if (this-> _rest.blockAddr != 0) {
      if (this-> _nbBlocks != 0) {
        for (uint32_t index = 0 ; index < this-> _nbBlocks ; index ++) {
          Allocator::instance ().freeFast (this-> _fstBlockAddr + index);
        }
      }

      Allocator::instance ().free (this-> _rest);

      this-> _rest = {0, 0};
      this-> _fstBlockAddr = 0;
      this-> _nbBlocks = 0;
      this-> _size = 0;
      this-> _sizeDividePerBlock = 0;
      this-> _sizePerBlock = 0;
    }
  }

  uint32_t CacheArrayBase::len () const {
    return this-> _size;
  }

  uint32_t CacheArrayBase::nbBlocks () const {
    if (this-> _size != 0) {
      return this-> _nbBlocks + 1;
    } else return 0;
  }

  CacheArrayBase::~CacheArrayBase () {
    this-> dispose ();
  }


}
