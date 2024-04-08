#pragma once

#include "gc.hh"
#include <gc/gc.h>

namespace rd_utils::memory {

  template <class X>
  void destruct_class (GC_PTR obj, GC_PTR x) {
    ((X*)obj)->~X ();
  }

  template <typename T>
  class GCBox {

    // The value inside the box
    T * _value;

  public:

    template <typename ... A>
    GCBox (A ... args) {
      this-> _value = new (GC) T (args...);
    }

    GCBox (T && val) {
      this-> _value = new (GC) T (std::move (val));
      GC_register_finalizer (this-> _value, destruct_class<T>, nullptr, nullptr, nullptr);
    }

    GCBox (const T & val) {
      this-> _value = new (GC) T (val);
      GC_register_finalizer (this-> _value, destruct_class<T>, nullptr, nullptr, nullptr);
    }

    T* get () {
      return this-> _value;
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
