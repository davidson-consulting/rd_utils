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

    FlatString (const std::string & value) {
      *this = value.c_str ();
    }

    /**
     * ============================================================================
     * ============================================================================
     * ================================    COPY   ================================
     * ============================================================================
     * ============================================================================
     * */

    void operator=(const char * value) {
      uint32_t i = 0;
      while (i < N && *value) {
        this-> _value [i] = *value;
        value ++;
        i += 1;
      }
      this-> _value [i] = 0;
      this-> _len = i;
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

    char operator[](uint32_t i) const {
      return this-> _value [i];
    }

    char & operator[](uint32_t i) {
      return this-> _value [i];
    }


    /**
     * ============================================================================
     * ============================================================================
     * =================================    CMP   =================================
     * ============================================================================
     * ============================================================================
     * */

    inline bool operator < (const std::string & o) const {
      return inf (o.c_str ());
    }

    inline bool operator > (const std::string & o) const {
      return sup (o.c_str ());
    }

    inline bool operator <= (const std::string & o) const {
      return inf_eq (o.c_str ());
    }

    inline bool operator >= (const std::string & o) const {
      return sup_eq (o.c_str ());
    }

    inline bool operator == (const std::string & o) const {
      return eq (o.c_str ());
    }

    inline bool operator < (const char * value) const {
      return inf (value);
    }

    inline bool operator > (const char * value) const {
      return sup (value);
    }

    inline bool operator <= (const char * value) const {
      return inf_eq (value);
    }

    inline bool operator >= (const char * value) const {
      return sup_eq (value);
    }

    inline bool operator == (const char * value) const {
      return eq (value);
    }

    template <int J>
    inline bool operator < (const FlatString<J> & o) const {
      return inf (o.c_str ());
    }

    template <int J>
    inline bool operator > (const FlatString<J> & o) const {
      return sup (o.c_str ());
    }

    template <int J>
    inline bool operator <= (const FlatString<J> & o) const {
      return inf_eq (o.c_str ());
    }

    template <int J>
    inline bool operator >= (const FlatString<J> & o) const {
      return sup_eq (o.c_str ());
    }

    template <int J>
    inline bool operator == (const FlatString<J> & o) const {
      return eq (o.c_str ());
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

    inline bool inf (const char * value) const {
      return this-> strcmp (this-> _value, value) < 0;
    }

    inline bool sup (const char * value) const {
      return this-> strcmp (this-> _value, value) > 0;
    }

    inline bool eq (const char * value) const {
      return this-> strcmp (this-> _value, value) == 0;
    }

    inline bool inf_eq (const char * value) const {
      return this-> strcmp (this-> _value, value) <= 0;
    }

    inline bool sup_eq (const char * value) const {
      return this-> strcmp (this-> _value, value) >= 0;
    }

    // Function to implement strcmp function
    inline int strcmp(const char *X, const char *Y) const {
      while (*X) {
        // if characters differ, or end of the second string is reached
        if (*X != *Y) {
          break;
        }

        // move to the next pair of characters
        X++;
        Y++;
      }

      // return the ASCII difference after converting `char*` to `unsigned char*`
      return *(const unsigned char*)X - *(const unsigned char*)Y;
    }

  };

}
