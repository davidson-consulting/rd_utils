#pragma once


#include <deque>
#include <optional>
#include <rd_utils/concurrency/mutex.hh>

namespace rd_utils::concurrency {

  /**
   * Mailbox used to send async messages between threads
   * Unlike IO pipe, mailbox are non blocking
   */
  template <typename T>
  class Mailbox {
  private:

    // The list of mails
    std::deque<T> _mails;

    // The len of the mailbox
    unsigned int _len;

    // The mutex to lock when submitting new values to the mailbox
    mutex _m;

  public:

    /**
     * Create a new empty mailbox
     */
    Mailbox () : _len (0) {}

    /**
     * Move ctor
     */
    Mailbox (Mailbox<T> && other) :
      _mails (std::move (other._mails)),
      _len (other._len)
    {
      other._len = 0;
    }

    /**
     * Move affectation
     */
    void operator=(Mailbox<T> && other) {
      this-> dispose ();
      this-> _mails = std::move (other._mails);
      this-> _len = other._len;
      other._len = 0;
    }
    /**
     * Post a message in the mailbox
     */
    void send (T value) {
      WITH_LOCK (this-> _m) {
        this-> _mails.push_back (std::move (value));
        this-> _len += 1;
      }
    }

    bool receive (T & val) {
      WITH_LOCK (this-> _m) {
        if (this-> _len == 0) {
          return false;
        } else {
          val = this-> _mails.front ();
          this-> _mails.pop_front ();
          this-> _len -= 1;
          return true;
        }
      }
    }

    /**
     * clear the messages in the mailbox
     */
    void clear () {
      WITH_LOCK (this-> _m) {
        std::deque<T> empty;
        std::swap (this-> _mails, empty);
        this-> _len = 0;
      }
    }

    /**
     * @returns: the number of elements in the mailbox
     */
    unsigned int len () {
      return this-> _len;
    }

    /**
     * Clear everything in the box
     */
    void dispose () {
      this-> clear ();
    }

    /**
     * this-> dispose ()
     */
    ~Mailbox () {
      this-> dispose ();
    }


  };


}
