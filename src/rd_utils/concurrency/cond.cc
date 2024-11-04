#include "cond.hh"


namespace rd_utils::concurrency {



  condition::condition () :
    _init (true)
  {
    ::pthread_cond_init (&this-> _cond, nullptr);
  }

  condition::condition (condition && other) :
    _init (other._init),
    _cond (other._cond)
  {
    other._init = false;
    other._cond = pthread_cond_t{0};
  }

  void condition::operator= (condition && other) {
    this-> dispose ();
    this-> _init = other._init;
    this-> _cond = other._cond;

    other._init = false;
    other._cond = pthread_cond_t{0};
  }

  void condition::wait (mutex & m) {
    ::pthread_cond_wait (&this-> _cond, m._m);
  }

  void condition::signal () {
    ::pthread_cond_signal (&this-> _cond);
  }

  void condition::dispose () {
    if (this-> _init) {
      this-> _init = false;
      this-> _cond = pthread_cond_t {0};
    }
  }

  condition::~condition () {
    this-> dispose ();
  }



}
