#pragma once

#include <rd_utils/foreign/queue.hh>

namespace rd_utils::concurrency {

  template<typename T>
  class LockFreeMailbox {
  private:

    lockfree::mpmc::Queue<T, 1024> _queue;

  public:

    LockFreeMailbox () {}

    LockFreeMailbox (LockFreeMailbox<T> && other) :
      _queue (std::move (other._queue))
    {}

    void operator=(LockFreeMailbox<T> && other) {
      this-> _queue = std::move (other._queue);
    }

    void send (T value) {
      this-> _queue.Push (value);
    }

    bool receive (T & value) {
      return this-> _queue.Pop (value);
    }

    void dispose () {
      this-> _queue = lockfree::mpmc::Queue <T, 1024> ();
    }

    ~LockFreeMailbox () {
      this-> dispose ();
    }

  };

}
