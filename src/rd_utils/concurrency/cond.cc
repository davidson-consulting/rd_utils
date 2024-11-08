#include "cond.hh"


namespace rd_utils::concurrency {

  condition::condition () {
    this-> _cond = new pthread_cond_t ();
    ::pthread_cond_init (this-> _cond, nullptr);
  }

  condition::condition (condition && other) :
    _cond (other._cond)
  {
    other._cond = nullptr;
  }

  void condition::operator= (condition && other) {
    this-> dispose ();
    this-> _cond = other._cond;

    other._cond = nullptr;
  }

  void condition::wait (mutex & m) {
    if (this-> _cond != nullptr) {
      ::pthread_cond_wait (this-> _cond, m._m);
    }
  }

  void condition::signal () {
    if (this-> _cond != nullptr) {
      ::pthread_cond_signal (this-> _cond);
    }
  }

  void condition::dispose () {
    if (this-> _cond != nullptr) {
      delete this-> _cond;
      this-> _cond = nullptr;
    }
  }

  condition::~condition () {
    this-> dispose ();
  }


}
