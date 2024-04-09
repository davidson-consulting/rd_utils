
#pragma once

#include <rd_utils/memory/cache/allocator.hh>
#include <rd_utils/utils/_.hh>
#include <cstring>

namespace rd_utils::memory::cache::collection {


template <typename T>
void printArray(T A[], int size)
{
    for (int i = 0; i < size; i++)
        std::cout << A[i] << " ";
    std::cout << std::endl;
}


  class CacheArrayBase {
  protected:

    // The memory segment of the array
    AllocatedSegment _rest;

    // The index of the first block
    uint32_t _fstBlockAddr;

    // The number of elements in the array
    uint32_t _size;

    // The size of inner elements
    uint32_t _innerSize;

    // The nb of blocks allocated especially for this array
    uint32_t _nbBlocks;

    // The size per block (without considering the rest)
    uint32_t _sizePerBlock;

    // The size per block (or 1, if there is only this-> _rest)
    uint32_t _sizeDividePerBlock;

  protected:

    CacheArrayBase (CacheArrayBase * other);
    void move (CacheArrayBase * other);

  public:

    CacheArrayBase ();

    CacheArrayBase (uint32_t size, uint32_t innerSize);

    void send (net::TcpStream & stream, uint32_t bufferSize);

    void recv (net::TcpStream & stream, uint32_t bufferSize);

    uint32_t len () const ;

    uint32_t nbBlocks () const;

    virtual ~CacheArrayBase ();

  private:

    void sendBlock (net::TcpStream & stream, AllocatedSegment seg, uint32_t nbElements, uint8_t * buffer, uint32_t nbInBuffer);

    void recvBlock (net::TcpStream & stream, AllocatedSegment seg, uint32_t nbElements, uint8_t * buffer, uint32_t nbInBuffer);

    void allocate (uint32_t size, uint32_t innerSize);

    void dispose ();

  };


  template <typename T>
  class CacheArray : public CacheArrayBase {
  private:

    CacheArray (const CacheArray<T> &);
    void operator=(const CacheArray<T>&);


  public:


    class Pusher {
    private:

      collection::CacheArray<T> * _context;

      uint32_t _beg;

      uint32_t _i;

      T * _buffer;

      uint32_t _bufferSize;

    public:

      Pusher (collection::CacheArray<T> * context, uint32_t i, T * buffer, uint32_t bufferSize) :
        _context (context)
        , _beg (i)
        , _i (0)
        , _buffer (buffer)
        , _bufferSize (bufferSize)
      {}

      bool push (const T & val) {
        this-> _buffer [this-> _i] = val;
        this-> _i++;
        if (this-> _i >= this-> _bufferSize) {
          return this-> commit ();
        }

        return true;
      }

      bool commit () {
        if (this-> _i != 0) {
          return this-> commitForce ();
        }
        return true;
      }

      ~Pusher () {
        if (this-> _i != 0) {
          this-> commitForce ();
        }
      }

    private:

      bool commitForce () {
        auto write = this-> _i < this-> _bufferSize ? this-> _i : this-> _bufferSize;
        if (write != 0) {
          this-> _context-> setNb (this-> _beg, this-> _buffer, write);
          this-> _beg += this-> _i;
          auto restLen = this-> _context-> len () - this-> _beg;
          this-> _bufferSize = this-> _bufferSize < restLen ? this-> _bufferSize : restLen;
          this-> _i = 0;
          return true;
        } else {
          this-> _bufferSize = 0;
          this-> _i = 0;
          this-> _beg = 0;
          return false;
        }
      }

    };

    class Puller {
    private:

      collection::CacheArray<T> * _context;

      uint32_t _beg;

      uint32_t _i;

      T * _buffer;

      uint32_t _bufferSize;

    public:

      Puller (collection::CacheArray<T> * context, uint32_t i, T * buffer, uint32_t bufferSize) :
        _context (context)
        , _beg (i)
        , _i (bufferSize - 1)
        , _buffer (buffer)
        , _bufferSize (bufferSize)
      {}

      const T & current () {
        return this-> _buffer [this-> _i];
      }

      bool next () {
        this-> _i++;
        if (this-> _i >= this-> _bufferSize) {
          return this-> retreive ();
        }
        return true;
      }

      void printBuffer () {
        for (uint32_t i = 0 ; i < this-> _bufferSize ; i++) {
          std::cout << this-> _buffer [i] << ", ";
        }
        std::cout << std::endl;
      }

    private:

      bool retreive () {
        auto read = std::min (this-> _bufferSize, this-> _context-> len () - this-> _beg);
        if (read == 0) return false;

        this-> _context-> getNb (this-> _beg, this-> _buffer, read);
        this-> _beg += read;
        this-> _bufferSize = read;
        this-> _i = 0;
        return true;
      }

    };

    class Slice {
    private:

      friend CacheArray<T>;

      CacheArray<T> * _context;

      uint32_t _beg, _len;

    public:

      Slice (CacheArray<T> * context, uint32_t beg, uint32_t len):
        _context (context)
        , _beg (beg)
        , _len (len)
      {}

      inline void set (uint32_t i, const T & val) {
        this-> _context-> set (i + this-> _beg, val);
      }

      inline T get (uint32_t i) {
        return this-> _context-> get (i + this-> _beg);
      }

      inline uint32_t len () const {
        return this-> _len;
      }

      inline void setNb (uint32_t i, T * buffer, uint32_t nb) {
        this-> _context-> setNb (i + this-> _beg, buffer, nb);
      }

      inline void getNb (uint32_t i, T * buffer, uint32_t nb) {
        this-> _context-> getNb (i + this-> _beg, buffer, nb);
      }

      inline void copy (collection::CacheArray<T>::Slice aux, T * buffer, uint32_t bufferSize) {
        this-> _context-> copy (this-> _beg, aux._context, aux._beg, std::min (this-> _len, aux._len), buffer, bufferSize);
      }

      inline void copy (collection::CacheArray<T>::Slice aux) {
        this-> _context-> copy (this-> _beg, aux-> _context, aux-> _beg, std::min (this-> _len, aux-> _len));
      }

      collection::CacheArray<T>::Puller puller (T * buffer, uint32_t bufferSize) {
        return this-> _context-> puller (this-> _beg, buffer, bufferSize);
      }

      collection::CacheArray<T>::Pusher pusher (T * buffer, uint32_t bufferSize) {
        return this-> _context-> pusher (this-> _beg, buffer, bufferSize);
      }

      bool equals (const std::vector <T> & vec, T * buffer, uint32_t nb) {
        auto puller = this-> puller (buffer, nb);
        puller.next ();

        for (auto & it : vec) {
          if (it != puller.current ()) return false;
          puller.next ();
        }

        return true;
      }

    };

  public:

    CacheArray (CacheArray <T> && other) :
      CacheArrayBase (&other)
    {}

    void operator= (CacheArray<T> && other) {
      this-> move (&other);
    }

    CacheArray () {}

    /**
     * Create a new array of a fixed size
     */
    CacheArray (uint32_t size) :
      CacheArrayBase (size, sizeof (T))
    {}

    collection::CacheArray<T>::Slice slice (uint32_t start, uint32_t end) {
      return collection::CacheArray<T>::Slice (this, start, end - start);
    }

    collection::CacheArray<T>::Puller puller (uint32_t start, T * buffer, uint32_t bufferSize) {
      return collection::CacheArray<T>::Puller (this, start, buffer, bufferSize);
    }

    collection::CacheArray<T>::Pusher pusher (uint32_t start, T * buffer, uint32_t bufferSize) {
      return collection::CacheArray<T>::Pusher (this, start, buffer, bufferSize);
    }

    /**
     * Access an element in the array as a lvalue
     */
    inline void set (uint32_t i, const T & val) {
      uint32_t absolute = i;
      AllocatedSegment seg = this-> _rest;

      int32_t index = absolute / (this-> _sizeDividePerBlock);
      uint32_t offset = (absolute - (index * this-> _sizeDividePerBlock));

      if (index < this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = ALLOC_HEAD_SIZE;
      } else if (this-> _sizeDividePerBlock == 1) {
        offset = index;
      }

      Allocator::instance ().write (seg, &val, offset * sizeof (T), sizeof (T));
    }

    /**
     * Access an element in the array
     */
    inline T get (uint32_t i) const {
      uint32_t absolute = i;
      AllocatedSegment seg = this-> _rest;

      int32_t index = absolute / (this-> _sizeDividePerBlock);
      uint32_t offset = (absolute - (index * this-> _sizeDividePerBlock));

      if (index < this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = ALLOC_HEAD_SIZE;
      } else if (this-> _sizeDividePerBlock == 1) {
        offset = index;
      }

      char buffer [sizeof (T)];
      Allocator::instance ().read (seg, buffer, offset * sizeof (T), sizeof (T));
      return *reinterpret_cast<T*> (buffer);
    }

    inline void setNb (uint32_t i, T * buffer, uint32_t nb) {
      uint32_t absolute = i;
      AllocatedSegment seg = this-> _rest;

      int32_t index = absolute / (this-> _sizeDividePerBlock);
      uint32_t offset = (absolute - (index * this-> _sizeDividePerBlock));

      if (index < this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = ALLOC_HEAD_SIZE;

        auto end = nb + offset;
        if (end > this-> _sizeDividePerBlock) {
          auto rest = end - this-> _sizeDividePerBlock;
          nb = nb - rest;
          setNb (i + nb, buffer + nb, rest);
        }

        Allocator::instance ().write (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
      } else {
        if (this-> _sizeDividePerBlock == 1) {
          offset = index;
        }

        Allocator::instance ().write (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
      }
    }

    inline void getNb (uint32_t i, T * buffer, uint32_t nb) const {
      uint32_t absolute = i;
      AllocatedSegment seg = this-> _rest;

      int32_t index = absolute / (this-> _sizeDividePerBlock);
      uint32_t offset = (absolute - (index * this-> _sizeDividePerBlock));

      if (index < this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = ALLOC_HEAD_SIZE;

        auto end = nb + offset;
        if (end > this-> _sizeDividePerBlock) {
          auto rest = end - this-> _sizeDividePerBlock;
          nb = nb - rest;
          getNb (i + nb, buffer + nb, rest);
        }

        Allocator::instance ().read (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
      } else {
        if (this-> _sizeDividePerBlock == 1) {
          offset = index;
        }
        Allocator::instance ().read (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
      }
    }

    template <typename F>
    void map (T * buffer, uint32_t bufferSize, F func) {
      std::list <uint32_t> toLoad;
      AllocatedSegment seg = {.blockAddr = 0, .offset = ALLOC_HEAD_SIZE};
      for (uint32_t i = 0 ; i < this-> _nbBlocks ; i++) {
        seg.blockAddr = this-> _fstBlockAddr + i;
        if (Allocator::instance ().isLoaded (seg.blockAddr)) {
          this-> mapBlock (seg, this-> _sizeDividePerBlock, buffer, bufferSize, func);
        } else {
          toLoad.push_back (i);
        }
      }

      auto globIndex = this-> _nbBlocks * this-> _sizeDividePerBlock;
      this-> mapBlock (this-> _rest, this-> _size - globIndex, buffer, bufferSize, func);
      for (auto & it : toLoad) {
        seg.blockAddr = this-> _fstBlockAddr + it;
        this-> mapBlock (seg, this-> _sizeDividePerBlock, buffer, bufferSize, func);
      }
    }

    template <typename F>
    void generate (T * buffer, uint32_t bufferSize, F func) {
      std::list <uint32_t> toLoad;
      AllocatedSegment seg = {.blockAddr = 0, .offset = ALLOC_HEAD_SIZE};
      uint64_t globIndex = 0;
      auto div = this-> _sizeDividePerBlock;

      for (uint32_t i = 0 ; i < this-> _nbBlocks ; i++) {
        seg.blockAddr = this-> _fstBlockAddr + i;
        if (Allocator::instance ().isLoaded (seg.blockAddr)) {
          globIndex = i * div;
          this-> generateBlock (seg, globIndex, div, buffer, bufferSize, func);
        } else {
          toLoad.push_back (i);
        }
      }

      globIndex = this-> _nbBlocks * div;
      this-> generateBlock (this-> _rest, globIndex, this-> _size - globIndex, buffer, bufferSize, func);
      for (auto & it : toLoad) {
        seg.blockAddr = this-> _fstBlockAddr + it;
        globIndex = it * div;
        this-> generateBlock (seg, globIndex, div, buffer, bufferSize, func);
      }
    }

    template <typename F, typename Z>
    Z reduce (T * buffer, uint32_t bufferSize, F func, Z fst) {
      Z result = fst;

      std::list <uint32_t> toLoad;
      AllocatedSegment seg = {.blockAddr = 0, .offset = ALLOC_HEAD_SIZE};
      uint64_t globIndex = 0, read = 0;
      for (uint32_t i = 0 ; i < this-> _nbBlocks ; i++) {
        seg.blockAddr = this-> _fstBlockAddr + i;
        if (Allocator::instance ().isLoaded (seg.blockAddr)) {
          globIndex = i * (this-> _sizePerBlock / sizeof (T));
          this-> reduceBlock (seg, globIndex, this-> _sizePerBlock / sizeof (T), result, buffer, bufferSize, func);
          read += (this-> _sizePerBlock / sizeof (T));
        } else {
          toLoad.push_back (this-> _fstBlockAddr + i);
        }
      }

      globIndex = this-> _nbBlocks * (this-> _sizePerBlock / sizeof (T));
      this-> reduceBlock (this-> _rest, globIndex, this-> _size - (globIndex), result, buffer, bufferSize, func);

      for (auto & it : toLoad) {
        seg.blockAddr = it;
        globIndex = it * (this-> _sizePerBlock / sizeof (T));
        this-> reduceBlock (seg, globIndex, this-> _sizePerBlock / sizeof (T), result, buffer, bufferSize, func);
      }

      return result;
    }

    void copyRaw (collection::CacheArray<T> & aux) {
      std::list <uint32_t> toLoad;
      AllocatedSegment seg = {.blockAddr = 0, .offset = ALLOC_HEAD_SIZE};
      for (uint32_t i = 0 ; i < this-> _nbBlocks ; i++) {
        seg.blockAddr = this-> _fstBlockAddr + i;
        auto auxSeg = aux._fstBlockAddr + i;
        if (Allocator::instance ().isLoaded (seg.blockAddr) || Allocator::instance ().isLoaded (auxSeg)) {
          Allocator::instance ().copy ({.blockAddr = auxSeg, .offset = seg.offset}, seg, this-> _sizePerBlock);
        } else { toLoad.push_back (i); }
      }

      Allocator::instance ().copy (aux._rest, this-> _rest, this-> _size * sizeof (T) - (this-> _nbBlocks * this-> _sizePerBlock));
      for (auto & it : toLoad) {
        seg.blockAddr = this-> _fstBlockAddr + it;
        auto auxSeg = aux._fstBlockAddr + it;
        Allocator::instance ().copy ({.blockAddr = auxSeg, .offset = seg.offset}, seg, this-> _sizePerBlock);
      }
    }

    bool equals (const std::vector <T> & vec, T * buffer, uint32_t nb) {
      auto puller = this-> puller (0, buffer, nb);
      puller.next ();

      for (auto & it : vec) {
        if (it != puller.current ()) return false;
        puller.next ();
      }

      return true;
    }

    inline void copy (uint32_t i, collection::CacheArray<T>::Slice aux, T * buffer, uint32_t bufferSize) {
      this-> copy (i, aux._context, aux._beg, std::min (aux._len, this-> _size - i), buffer, bufferSize);
    }

    inline void copy (uint32_t i, collection::CacheArray<T>::Slice aux) {
      this-> copy (i, aux._context, aux._beg, std::min (aux._len, this-> _size - i));
    }

  private :

    /**
     * Copy elements from the aux array into this array using a buffer to accelerate the copy and minimize the number of allocator access
     * @params:
     *    - i: the index where to place the data (in this)
     *    - j: the index where to get the data (in aux)
     *    - nb: the number of elements to copy
     *    - buffer: the buffer used to make the copy
     *    - bufferSize the size of the buffer used for the copy
     */
    void copy (uint32_t i, collection::CacheArray<T> * aux, uint32_t j, uint32_t nb, T * buffer, uint32_t bufferSize) {
      if (bufferSize == 0 || bufferSize == 1) {
        this-> copy (i, aux, j, nb);
      }

      auto aligned = (nb / bufferSize) * bufferSize;
      auto rest = nb - aligned;

      for (uint32_t k = 0 ; k < aligned ; k += bufferSize) {
        aux-> getNb (k + j, buffer, bufferSize);
        this-> setNb (k + i, buffer, bufferSize);
      }

      if (rest != 0) {
        aux-> getNb (aligned + j, buffer, rest);
        this-> setNb (aligned + i, buffer, rest);
      }
    }

    /**
     * Copy elements from the aux array into this array without an acceleration buffer
     * @params:
     *    - i: the index where to place the data (in this)
     *    - j: the index where to get the data (in aux)
     *    - nb: the number of elements to copy
     */
    void copy (uint32_t i, collection::CacheArray<T> * aux, uint32_t j, uint32_t nb) {
      for (uint32_t k = 0 ; k < nb ; k++) {
        this-> set (k + i, aux-> get (k + j));
      }
    }

    template <typename F>
    void mapBlock (AllocatedSegment seg, uint32_t nbElements, T * buffer, uint32_t bufferSize, F func) {
      for (uint32_t i = 0 ; i < nbElements ; i += bufferSize) {
        auto nb = nbElements - i > bufferSize ? bufferSize : nbElements - i;
        Allocator::instance ().read (seg, buffer,  i * sizeof (T), sizeof (T) * nb);
        for (uint32_t j = 0 ; j < nb ; j++) {
          buffer [j] = func (buffer [j]);
        }

        Allocator::instance ().write (seg, buffer,  i * sizeof (T), sizeof (T) * nb);
      }
    }

    template <typename F>
    void generateBlock (AllocatedSegment seg, uint64_t globIndex, uint32_t nbElements, T * buffer, uint32_t bufferSize, F func) {
      for (uint32_t i = 0 ; i < nbElements ; i += bufferSize) {
        auto nb = nbElements - i > bufferSize ? bufferSize : nbElements - i;

        for (uint32_t j = 0 ; j < nb ; j++) {
          buffer [j] = func (globIndex  + i + j);
        }

        Allocator::instance ().write (seg, buffer,  i * sizeof (T), sizeof (T) * nb);
      }
    }

    template <typename Z, typename F>
    void reduceBlock (AllocatedSegment seg, uint64_t globIndex, uint32_t nbElements, Z & result, T * buffer, uint32_t bufferSize, F func) {
      for (uint32_t i = 0 ; i < nbElements ; i += bufferSize) {
        auto nb = (nbElements - i) > bufferSize ? bufferSize : (nbElements - i);

        Allocator::instance ().read (seg, buffer,  i * sizeof (T), sizeof (T) * nb);
        for (uint64_t j = 0 ; j < nb ; j++) {
          result = func (result, buffer [j]);
        }
      }
    }

  };

}

template <typename T>
std::ostream& operator << (std::ostream & s, rd_utils::memory::cache::collection::CacheArray<T> & array) {
  uint32_t BLK_SIZE = 8192;
  T * buffer = new T [BLK_SIZE];
  auto len = array.len ();
  uint32_t i = 0;
  auto p = array.puller (0, buffer, BLK_SIZE);
  s << "[";
  p.next ();
  while (i < len) {
    if (i != 0) s << ", ";
    s << p.current ();
    p.next ();
    i ++;
  }
  s << "]";

  delete [] buffer;
  return s;
}
