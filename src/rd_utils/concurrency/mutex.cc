#include <rd_utils/concurrency/mutex.hh>

namespace rd_utils::concurrency {

  mutex::mutex () : _m (PTHREAD_MUTEX_INITIALIZER)
  {}

  void mutex::lock () {
    pthread_mutex_lock (&this-> _m);
  }

  void mutex::unlock () {
    pthread_mutex_unlock (&this-> _m);
  }

  mutex::~mutex () {
    this-> unlock ();
  }

  mutex_locker::mutex_locker (mutex & m) :
    _m (m)
  {
    m.lock ();
  }

  mutex_locker::~mutex_locker () {
    this-> _m.unlock ();
  }

	
}


