#include "semaphore.hh"
#include <semaphore.h>

namespace rd_utils::concurrency {


  semaphore::semaphore (int nb_resources) {
    ::sem_init (&this-> _sem, 0, nb_resources);
    this-> _init = true;
  }

  semaphore::semaphore (semaphore && other) :
    _sem (other._sem),
    _init (other._init)
    {
      other._init = false;
      other._sem = sem_t{0};
    }


  void semaphore::operator=(semaphore && other) {
    this-> dispose ();
    this-> _init = other._init;
    this-> _sem = other._sem;

    other._init = false;
    other._sem = sem_t{0};
  }

  void semaphore::dispose () {
    if (this-> _init) {
      ::sem_destroy (&this-> _sem);
      this-> _init = false;
      this-> _sem = sem_t{0};
    }
  }

  void semaphore::post () {
    ::sem_post (&this-> _sem);
  }

  void semaphore::wait () {
    ::sem_wait (&this-> _sem);
  }

  semaphore::~semaphore () {
    this-> dispose ();
  }

}
