#include "tpipe.hh"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

namespace rd_utils::concurrency {

  ThreadPipe::ThreadPipe (bool create) :
    _s (0)
  {
    if (create) {
      int pipes[2];
      ::pipe (pipes);

      this-> _read = pipes [0];
      this-> _write = pipes [1];
    }
  }

  ThreadPipe::ThreadPipe (ThreadPipe && other) :
    _read (other._read),
    _write (other._write),
    _m (std::move (other._m)),
    _s (std::move (other._s)),
    _c (std::move (other._c))
  {
    other._read = 0;
    other._write = 0;
  }


  void ThreadPipe::operator=(ThreadPipe && other) {
    this-> dispose ();

    this-> _read = other._read;
    this-> _write = other._write;
    this-> _m = std::move (other._m);
    this-> _s = std::move (other._s);
    this-> _c = std::move (other._c);

    other._read = 0;
    other._write = 0;
  }

  int ThreadPipe::getWriteFd () const {
    return this-> _write;
  }

  int ThreadPipe::getReadFd () const {
    return this-> _read;
  }

  void ThreadPipe::setNonBlocking () {
    auto old_flg_rd = fcntl (this-> _read, F_GETFL, 0);
    fcntl (this-> _read, F_SETFL, old_flg_rd | O_NONBLOCK);

    auto old_flg_wr = fcntl (this-> _write, F_GETFL, 0);
    fcntl (this-> _write, F_SETFL, old_flg_wr | O_NONBLOCK);
  }

  void ThreadPipe::dispose () {
    if (this-> _read != 0) {
      ::close (this-> _read);
      ::close (this-> _write);

      this-> _read = 0;
      this-> _write = 0;
    }
  }

  ThreadPipe::~ThreadPipe () {
    this-> dispose ();
  }

}
