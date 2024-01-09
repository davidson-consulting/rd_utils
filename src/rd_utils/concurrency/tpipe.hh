#pragma once

#include "mutex.hh"
#include "cond.hh"
#include "semaphore.hh"
#include <rd_utils/utils/error.hh>

#include <unistd.h>

namespace rd_utils::concurrency {

  /**
   * Pipe used to communicate between threads
   */
  class ThreadPipe {

    int _read = 0;

    int _write = 0;

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
     * @returns: the output file descriptor
     */
    int getWriteFd () const;

    /**
     * @returns: the input file descriptor
     */
    int getReadFd () const;

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
        int r = ::read (this-> _read, x, sizeof (T*));
        if (r != sizeof (T*)) {
          throw utils::Rd_UtilsError ("Failed to read from pipe");
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

        int r = ::write (this-> _write, &box, sizeof (T*));
        this-> _c.signal ();
        if (r != sizeof (T*)) {
          throw utils::Rd_UtilsError ("Failed to write on pipe");
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
