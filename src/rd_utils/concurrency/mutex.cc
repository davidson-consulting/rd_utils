#include <rd_utils/concurrency/mutex.hh>
#include <memory>

namespace rd_utils::concurrency {

  mutex::mutex () :
    _m (new pthread_mutex_t ())
  {
    pthread_mutex_init (this-> _m, nullptr);
  }

  mutex::mutex (mutex && other) :
    _m (other._m)
  {
    other._m = nullptr;
  }

  void mutex::operator= (mutex && other) {
    if (this-> _m != nullptr) {
      delete this-> _m;
    }

    this-> _m = other._m;
    other._m = nullptr;
  }


  void mutex::lock () {
    if (this-> _m != nullptr) {
      pthread_mutex_lock (this-> _m);
    }
  }

  void mutex::unlock () {
    if (this-> _m != nullptr) {
      pthread_mutex_unlock (this-> _m);
    }
  }

  mutex::~mutex () {
    if (this-> _m != nullptr) {
      delete this-> _m;
      this-> _m = nullptr;
    }
  }

  mutex_locker::mutex_locker (mutex& m) :
    _m (m)
  {
    m.lock ();
  }

  mutex_locker::~mutex_locker () {
    this-> _m.unlock ();
  }

	
}


