#pragma once

#include "gc.hh"
#include <gc/gc.h>

namespace rd_utils::memory {

  template <class X>
  void destruct_class (GC_PTR obj, GC_PTR x) {
    ((X*)obj)->~X ();
  }

  template <typename T>
  class Box {

    // The value inside the box
    T * _value;

  public:

    Box (T && val) {
      this-> _value = new (GC) T (std::move (val));
      GC_register_finalizer (this-> _value, destruct_class<T>, nullptr, nullptr, nullptr);
    }

    Box (const T & val) {
      this-> _value = new (GC) T (val);
      GC_register_finalizer (this-> _value, destruct_class<T>, nullptr, nullptr, nullptr);
    }

    T& operator*() {
      return *this-> _value;
    }

    const T& operator*() const {
      return *this-> _value;
    }

    T* operator-> () {
      return this-> _value;
    }

    const T* operator-> () const {
      return this-> _value;
    }

  };

}
