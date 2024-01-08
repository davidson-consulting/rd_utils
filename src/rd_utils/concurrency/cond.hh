#pragma once

#include "mutex.hh"

#include <pthread.h>


namespace rd_utils::concurrency {


  /**
   * Condition are used to put thread in wait mode, until a particular condition occurs.
   */
  class condition {

    bool _init = false;
    pthread_cond_t _cond;

  private:

    condition (const condition & other);
    void operator=(const condition & other);

  public:

    /**
     * Create a condition
     */
    condition ();

    /**
     * Move ctor
     */
    condition (condition && other);

    /**
     * Move affectation
     */
    void operator=(condition && other);

    /**
     * Wait for the condition to occur
     * @params:
     *    - m: the mutex to associate to the condition
     */
    void wait (mutex & m);

    /**
     * Signal the condition, making the waiting thread continue
     */
    void signal ();

    /**
     * Clear the condition
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~condition ();

  };


}
