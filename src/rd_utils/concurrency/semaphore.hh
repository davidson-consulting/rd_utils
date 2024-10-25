#pragma once

// #include <pthread.h>
#include <semaphore.h>

namespace rd_utils::concurrency {

    /**
     * Semaphore are used to synchronize basic threads
     * Semaphore are close to Mutex, with the difference that a given number of thread can access the resource at the same time.
     */
    class semaphore {

        sem_t _sem;

        bool _init = false;

    private :

        semaphore (const semaphore & other);
        void operator=(const semaphore & other);

    public:

        /**
         */
        semaphore ();

        /**
         * Move ctor
         */
        semaphore (semaphore && other);

        /**
         * Move affectation
         */
        void operator=(semaphore && other);

        /**
         * Post the semaphor to signal that a certain section of code is being exited, releasing a slot.
         * @assume: self.wait () was called before the access
         */
        void post ();

        /**
         * Wait until there is a resource available in the semaphore or timeout
         * @params:
         *    - timeout: the timeout in second, -1 means forever
         * @returns: false if timeouted
         */
        bool wait (float timeout = -1);

        /**
         * @returns: the current count of the semaphore
         */
        int get ();

        /**
         * Dispose the semaphore
         */
        void dispose ();

        /**
         * this-> dispose ();
         */
        ~semaphore ();

    };



}
