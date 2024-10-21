#include "tpipe.hh"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

namespace rd_utils::concurrency {

  ThreadPipe::ThreadPipe (bool create) :
    _pipes (create)
  {}

  ThreadPipe::ThreadPipe (ThreadPipe && other) :
    _pipes (std::move (other._pipes)),
    _m (std::move (other._m)),
    _c (std::move (other._c)),
    _s (std::move (other._s))
  {}

  void ThreadPipe::operator=(ThreadPipe && other) {
    this-> dispose ();

    this-> _pipes = std::move (other._pipes);
    this-> _m = std::move (other._m);
    this-> _s = std::move (other._s);
    this-> _c = std::move (other._c);
  }

  IPipe & ThreadPipe::ipipe () {
    return this-> _pipes.ipipe ();
  }

  OPipe & ThreadPipe::opipe () {
    return this-> _pipes.opipe ();
  }

  void ThreadPipe::setNonBlocking () {
    this-> _pipes.opipe ().setNonBlocking ();
    this-> _pipes.ipipe ().setNonBlocking ();
  }

  void ThreadPipe::dispose () {
    this-> _pipes.dispose ();
  }

  ThreadPipe::~ThreadPipe () {
    this-> dispose ();
  }

}
