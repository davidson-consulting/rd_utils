#pragma once

#include "mutex.hh"
#include "cond.hh"
#include "semaphore.hh"
#include "iopipe.hh"

#include <rd_utils/utils/error.hh>

#include <unistd.h>

namespace rd_utils::concurrency {

  class IOPipe;

  /**
   * Pipe used to communicate between threads
   */
  class ThreadPipe {

    IOPipe _pipes;

    mutex _m;

    condition _c;

    semaphore _s;

  private:

    ThreadPipe (const ThreadPipe & other);
    void operator=(const ThreadPipe & other);

  public:

    /**
     * New thread pipe
     * @params:
     *    - create: if false, then does no create a working pipe
     */
    ThreadPipe (bool create = true);

    /**
     * Move ctor
     */
    ThreadPipe (ThreadPipe && other);

    /**
     * Move affectation
     */
    void operator=(ThreadPipe && other);

    /**
     * @returns: the read pipe
     */
    IPipe & ipipe ();

    /**
     * @returns: the write pipe
     */
    OPipe & opipe ();

    /**
     * Read an object from the pipe
     * @warning: this is a blocking process
     */
    template<typename T>
    T read () {
      T * z;
      T ** x = &z;

      WITH_LOCK (this-> _m) {
        this-> _s.post ();
        this-> _c.wait (this-> _m);
        if (!this-> _pipes.opipe ().writeRaw (x, sizeof (T*), false)) {
          throw std::runtime_error ("Failed to read on pipe");
        }

        T ret = *z;
        delete z;

        return ret;
      }
    }

    /**
     * Write an object in the pipe
     * @warning: this is a blocking process
     */
    template <typename T>
    void write (const T & data) {
      this-> _s.wait ();
      WITH_LOCK (this-> _m) {
        T* box = new T (data);

        auto succ = this-> _pipes.ipipe ().readRaw (&box, sizeof (T*));
        this-> _c.signal ();

        if (!succ) {
          throw std::runtime_error ("Failed to write on pipe");
        }
      }
    }

    /**
     * Set the pipe in non blocking mode
     */
    void setNonBlocking ();

    /**
     * Dispose the pipe
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~ThreadPipe ();

  };


}
