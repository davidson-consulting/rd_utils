#include "semaphore.hh"
#include <semaphore.h>
#include <cstdint>
#include <time.h>
#include <exception>
#include <stdexcept>
#include <iostream>

namespace rd_utils::concurrency {


  semaphore::semaphore () {
    this-> _sem = new sem_t ();
    ::sem_init (this-> _sem, 0, 0);
  }

  semaphore::semaphore (semaphore && other)
    : _sem (other._sem)
  {
    other._sem = nullptr;
  }


  void semaphore::operator=(semaphore && other) {
    this-> dispose ();
    this-> _sem = other._sem;

    other._sem = nullptr;
  }

  void semaphore::dispose () {
    if (this-> _sem != nullptr) {
      ::sem_destroy (this-> _sem);
      delete this-> _sem;
      this-> _sem = nullptr;
    }
  }

  void semaphore::post () {
    if (this-> _sem != nullptr) {
      ::sem_post (this-> _sem);
    }
  }

  bool semaphore::wait (float timeout) {
    if (this-> _sem == nullptr) return false;
    if (timeout < 0) {
      ::sem_wait (this-> _sem);
      return true;
    } else {
      timespec ts;
      if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        throw std::runtime_error ("Failed to wait semaphore, clock is wrong !!");
      }

      ts.tv_sec += (uint64_t) timeout;
      ts.tv_nsec += (uint64_t) (timeout * 1000000000) % 1000000000;

      if (ts.tv_nsec > 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec += 1;
      }

      return  ::sem_timedwait (this-> _sem, &ts) == 0;
    }
  }

  int semaphore::get () {
    if (this-> _sem == nullptr) return 0;
    int i = 0;
    ::sem_getvalue (this-> _sem, &i);

    return i;
  }

  semaphore::~semaphore () {
    this-> dispose ();
  }

}
