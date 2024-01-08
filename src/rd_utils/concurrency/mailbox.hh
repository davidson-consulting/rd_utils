#pragma once


#include <queue>
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
    std::queue<T> _mails;

    // The mutex to lock when submitting new values to the mailbox
    mutex _m;

  public:

    /**
     * Create a new empty mailbox
     */
    Mailbox () {}

    /**
     * Move ctor
     */
    Mailbox (Mailbox<T> && other) :
      _mails (std::move (other._mails))
    {}

    /**
     * Move affectation
     */
    void operator=(Mailbox<T> && other) {
      this-> dispose ();
      this-> _mails = std::move (other);
    }
    /**
     * Post a message in the mailbox
     */
    void send (T && value) {
      WITH_LOCK (this-> _m) {
        this-> _mails.push (std::move (value));
      }
    }

    /**
     * Retreive a message from the mailbox
     */
    std::optional<T> receive () {
      WITH_LOCK (this-> _m) {
        if (this-> _mails.empty ()) {
          return std::nullopt;
        } else {
          auto & ret = this-> _mails.back ();
          this-> _mails.pop ();
          return ret;
        }
      }
    }

    /**
     * clear the messages in the mailbox
     */
    void clear () {
      WITH_LOCK (this-> _m) {
        this-> _mails.clear ();
      }
    }

    /**
     * @returns: the number of elements in the mailbox
     */
    unsigned int len () {
      WITH_LOCK (this-> _m) {
        return this-> _mails.size ();
      }
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



#endif // MAILBOX_H_
