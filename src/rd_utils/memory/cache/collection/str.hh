#pragma once

#include <cstdint>
#include <string>
#include <iostream>

namespace rd_utils::memory::cache::collection {

  template <int N>
  class FlatString {

    char _value [N];

    uint32_t _len;

  public:

    /**
     * ============================================================================
     * ============================================================================
     * ================================    CTORS   ================================
     * ============================================================================
     * ============================================================================
     * */

    FlatString () {
      this-> _value [0] = 0;
    }

    FlatString (const char * value) {
      *this = value;
    }

    /**
     * ============================================================================
     * ============================================================================
     * ================================    COPY   ================================
     * ============================================================================
     * ============================================================================
     * */

    void operator=(const char * value) {
      for (uint32_t i = 0 ; i < N - 1; i++) {
        this-> _value [i] = value [i];
        if (value [i] == '\0') {
          this-> _len = i;
          break;
        }
      }
      this-> _value [this-> _len] = 0;
    }

    void operator=(const std::string & cst) {
      *this = cst.c_str ();
    }

    /**
     * ============================================================================
     * ============================================================================
     * ===============================    GETTERS   ===============================
     * ============================================================================
     * ============================================================================
     * */

    const char* c_str () const {
      return this-> _value;
    }

    uint32_t len () const {
      return this-> _len;
    }

    char operator[](uint32_t i) {
      return this-> _value [i];
    }

    /**
     * ============================================================================
     * ============================================================================
     * =================================    CMP   =================================
     * ============================================================================
     * ============================================================================
     * */

    int operator < (const std::string & o) const {
      return inf (o.c_str (), o.length ());
    }

    int operator > (const std::string & o) const {
      return sup (o.c_str (), o.length ());
    }

    int operator <= (const std::string & o) const {
      return inf_eq (o.c_str (), o.length ());
    }

    int operator >= (const std::string & o) const {
      return sup_eq (o.c_str (), o.length ());
    }

    int operator == (const std::string & o) const {
      return eq (o.c_str (), o.length ());
    }

    int operator < (const char * value) const {
      return inf (value, strlen (value));
    }

    int operator > (const char * value) const {
      return sup (value, strlen (value));
    }

    int operator <= (const char * value) const {
      return inf_eq (value, strlen (value));
    }

    int operator >= (const char * value) const {
      return sup_eq (value, strlen (value));
    }

    int operator == (const char * value) const {
      return eq (value, strlen (value));
    }

    template <int J>
    int operator < (const FlatString<J> & o) const {
      return inf (o.c_str (), o.len ());
    }

    template <int J>
    int operator > (const FlatString<J> & o) const {
      return sup (o.c_str (), o.len ());
    }

    template <int J>
    int operator <= (const FlatString<J> & o) const {
      return inf_eq (o.c_str (), o.len ());
    }

    template <int J>
    int operator >= (const FlatString<J> & o) const {
      return sup_eq (o.c_str (), o.len ());
    }

    template <int J>
    int operator == (const FlatString<J> & o) const {
      return eq (o.c_str (), o.len ());
    }


    /**
     * ============================================================================
     * ============================================================================
     * ================================    PRINT   ================================
     * ============================================================================
     * ============================================================================
     * */

    friend std::ostream & operator<< (std::ostream & o, const FlatString<N> & s) {
      o << s._value;
      return o;
    }


  private:

    bool inf (const char * value, uint32_t len) const {
      auto minLen = this-> _len < len ? this-> _len : len;
      for (uint32_t i = 0 ; i < minLen ; i++) {
        if (this-> _value [i] < value [i]) return true;
        else if (this-> _value [i] > value [i]) return false;
      }

      if (this-> _len < len) return true;
      return false;
    }

    bool sup (const char * value, uint32_t len) const {
      auto minLen = this-> _len < len ? this-> _len : len;
      for (uint32_t i = 0 ; i < minLen ; i++) {
        if (this-> _value [i] < value [i]) return false;
        else if (this-> _value [i] > value [i]) return true;
      }

      if (this-> _len > len) return true;
      return false;
    }

    bool eq (const char * value, uint32_t len) const {
      if (this-> _len != len) return false;
      for (uint32_t i = 0 ; i < len ; i++) {
        if (this-> _value [i] != value [i]) return false;
      }

      return true;
    }

    bool inf_eq (const char * value, uint32_t len) const {
      auto minLen = this-> _len < len ? this-> _len : len;
      for (uint32_t i = 0 ; i < minLen ; i++) {
        if (this-> _value [i] < value [i]) return true;
        else if (this-> _value [i] > value [i]) return false;
      }

      if (this-> _len <= len) return true;
      return false;
    }

    bool sup_eq (const char * value, uint32_t len) const {
      auto minLen = this-> _len < len ? this-> _len : len;
      for (uint32_t i = 0 ; i < minLen ; i++) {
        if (this-> _value [i] < value [i]) return false;
        else if (this-> _value [i] > value [i]) return true;
      }

      if (this-> _len >= len) return true;
      return false;
    }

  };


}
