#pragma once


#include <rd_utils/memory/cache/allocator.hh>
#include <rd_utils/net/_.hh>
#include <rd_utils/utils/_.hh>
#include <cstring>

namespace rd_utils::memory::cache::collection {

  class ArrayListBase {
  protected:

    // List of allocated blocks
    std::vector<uint32_t> _metadata;

    // The number of elements actually contained in the array
    uint32_t _size;

    // The size of an element
    uint32_t _innerSize;

    // The number of element in each blocks
    uint32_t _allocable;

  protected:

    ArrayListBase (ArrayListBase * other);
    void move (ArrayListBase * other);

    ArrayListBase ();

  public:

    /**
     * @params:
     *    - nbBlocks: number of blocks to pre allocate
     */
    ArrayListBase (uint32_t nbBlocks, uint32_t innerSize);

    /**
     * @returns: the number of elements stored in the array
     */
    uint32_t len () const;

    /**
     * @returns: the number of blocks stored in the array
     */
    uint32_t nbBlocks () const;

    void send (net::TcpStream & stream, uint32_t bufferSize);

    /**
     * this-> dispose ()
     */
    virtual ~ArrayListBase ();

  protected:

    /**
     * Allocate a new block
     */
    void grow ();

    /**
     * Clear everything
     */
    void dispose ();

    void sendBlock (net::TcpStream & stream, AllocatedSegment seg, uint32_t nbElements, uint8_t * buffer, uint32_t nbInBuffer);
  };

  template <typename T>
  class CacheArrayList : public ArrayListBase {

    class Puller {
    private:

      collection::CacheArrayList<T> * _context;
      uint32_t _beg;
      uint32_t _i;
      T* _buffer;
      uint32_t _bufferSize;

    public:

      Puller (CacheArrayList<T> * context, uint32_t i, T * buffer, uint32_t bufferSize) :
        _context (context)
        , _beg (i)
        , _i (bufferSize - 1)
        , _buffer (buffer)
        , _bufferSize (bufferSize)
      {}

      const T& current () {
        return this-> _buffer [this-> _i];
      }

      bool next () {
        this-> _i++;
        if (this-> _i >= this-> _bufferSize) {
          return this-> retreive ();
        }

        return true;
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

    class Pusher {
    private:

      collection::CacheArrayList<T> * _context;
      uint32_t _beg;
      uint32_t _i;
      T * _buffer;
      uint32_t _bufferSize;

    public:

      Pusher (collection::CacheArrayList<T> * context, uint32_t i, T * buffer, uint32_t bufferSize) :
        _context (context)
        , _beg (i)
        , _i (0)
        , _buffer (buffer)
        , _bufferSize (bufferSize)
      {}

      void push (const T & val) {
        this-> _buffer [this-> _i] = val;
        this-> _i++;
        if (this-> _i >= this-> _bufferSize) {
          this-> commit ();
        }
      }

      void commit () {
        if (this-> _i != 0) {
          this-> commitForce ();
        }
      }

      ~Pusher () {
        if (this-> _i != 0) {
          this-> commitForce ();
        }
      }

    private:

      void commitForce () {
        this-> _context-> pushOrSetNb (this-> _beg, this-> _buffer, this-> _i);
        this-> _beg += this-> _i;
        this-> _i = 0;
      }

    };


  public:

    CacheArrayList (CacheArrayList<T> && other) :
      ArrayListBase (&other)
    {}

    void operator= (CacheArrayList<T> && other) {
      this-> move (&other);
    }

    CacheArrayList () :
      ArrayListBase (0, sizeof (T))
    {}

    /**
     * Create an array puller to read the array sequentially
     */
    collection::CacheArrayList<T>::Puller puller (uint32_t start, T * buffer, uint32_t bufferSize) {
      return collection::CacheArrayList<T>::Puller (this, start, buffer, bufferSize);
    }

    collection::CacheArrayList<T>::Pusher pusher (uint32_t start, T * buffer, uint32_t bufferSize) {
      return collection::CacheArrayList<T>::Pusher (this, start, buffer, bufferSize);
    }

    /**
     * Write an element in the array
     */
    inline void set (uint32_t i, const T & val) {
      int32_t index = i / this-> _allocable;
      uint32_t offset = (i - (index * this-> _allocable));

      if (index >= this-> _metadata.size ()) std::runtime_error ("Out of bounds");

      AllocatedSegment seg = {.blockAddr = this-> _metadata [index], .offset = ALLOC_HEAD_SIZE};
      Allocator::instance ().write (seg, &val, offset * sizeof (T), sizeof (T));
    }

    /**
     * Read an element in the array
     */
    inline void get (uint32_t i, const T & val) {
      int32_t index = i / this-> _allocable;
      uint32_t offset = (i - (index * this-> _allocable));

      if (index >= this-> _metadata.size ()) throw std::runtime_error ("Out of bounds");

      AllocatedSegment seg = {.blockAddr = this-> _metadata [index], .offset = ALLOC_HEAD_SIZE};

      char buffer [sizeof (T)];
      Allocator::instance ().read (seg, buffer, offset * sizeof (T), sizeof (T));
      return *reinterpret_cast <T*> (buffer);
    }


    /**
     * Write nb elements in the array
     * @params:
     *    - i: the index of the first write
     *    - buffer: the buffer used to write
     *    - nb: the number of elements contained in the buffer (assuming buffer can contains at least /nb/ * sizeof (T))
     */
    inline void setNb (uint32_t i, T * buffer, uint32_t nb) {
      uint32_t index = i / this-> _allocable;
      uint32_t offset = (i - (index * this-> _allocable));

      if (index >= this-> _metadata.size ()) std::runtime_error ("Out of bounds");
      AllocatedSegment seg = {.blockAddr = this-> _metadata [index], .offset = ALLOC_HEAD_SIZE};
      auto end = nb + offset;
      if (end > this-> _allocable) {
        auto rest = end - this-> _allocable;
        nb = nb - rest;
        Allocator::instance ().write (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
        setNb (i + nb, buffer + nb, rest);
      } else {
        Allocator::instance ().write (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
      }
    }

    /**
     * Push a value in the array
     * @params:
     *   - val: the value to write at the end of the array
     */
    inline void push (const T & val) {
      uint32_t index = this-> _size / this-> _allocable;
      uint32_t offset = (this-> _size - (index * this-> _allocable));

      if (index >= this-> _metadata.size ()) {
        this-> grow ();
      }

      this-> _size += 1;
      AllocatedSegment seg = {.blockAddr = this-> _metadata [index], .offset = ALLOC_HEAD_SIZE};
      Allocator::instance ().write (seg, &val, offset * sizeof (T), sizeof (T));
    }

    /**
     * Push or overwrite nb elements in the array
     */
    inline void pushOrSetNb (uint32_t i, T * buffer, uint32_t nb) {
      if (i < this-> _size) {
        uint32_t before = this-> _size - i > nb ? nb : this-> _size - i;
        uint32_t after = nb - before;
        this-> setNb (i, buffer, before);
        if (after != 0) this-> pushNb (buffer + before, after);
      } else {
        this-> pushNb (buffer, nb);
      }
    }

    /**
     * Push nb elements in the array
     * @params:
     *    - buffer: the buffer used to write
     *    - nb: the number of elements contained in the buffer (assuming buffer can contains at least /nb/ * sizeof (T))
     */
    inline void pushNb (T * buffer, uint32_t nb) {
      uint32_t index = this-> _size / this-> _allocable;
      uint32_t offset = (this-> _size - (index * this-> _allocable));

      if (index >= this-> _metadata.size ()) {
        this-> grow ();
      }

      AllocatedSegment seg = {.blockAddr = this-> _metadata [index], .offset = ALLOC_HEAD_SIZE};
      auto end = nb + offset;
      if (end > this-> _allocable) {
        auto rest = end - this-> _allocable;
        nb = nb - rest;
        this-> _size += nb;

        Allocator::instance ().write (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
        pushNb (buffer + nb, rest);
      } else {
        this-> _size += nb;
        Allocator::instance ().write (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
      }
    }

    /**
     * Read nb elements in the array
     * @params:
     *    - i: the index of the first read
     *    - buffer: the buffer used to read
     *    - nb: the number of elements to read (assuming buffer can contains at least /nb/ * sizeof (T))
     */
    inline void getNb (uint32_t i, T * buffer, uint32_t nb) {
      uint32_t index = i / this-> _allocable;
      uint32_t offset = (i - (index * this-> _allocable));

      if (index >= this-> _metadata.size ()) throw std::runtime_error ("Out of bounds");

      AllocatedSegment seg = {.blockAddr = this-> _metadata [index], .offset = ALLOC_HEAD_SIZE};
      auto end = nb + offset;
      if (end > this-> _allocable) {
        auto rest = end - this-> _allocable;
        nb = nb - rest;
        Allocator::instance ().read (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
        getNb (i + nb, buffer + nb, rest);
      } else {
        Allocator::instance ().read (seg, buffer, offset * sizeof (T), sizeof (T) * nb);
      }
    }

  };

}

template <typename T>
std::ostream& operator<< (std::ostream & s, rd_utils::memory::cache::collection::CacheArrayList<T> & array) {
  uint32_t BLK_SIZE = 8192 / sizeof (T);
  T * buffer = new T [BLK_SIZE];
  uint32_t len = array.len ();
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
