#pragma once

#include <chrono>

namespace rd_utils {

    namespace concurrency {

	class timer {
	private : 

	    std::chrono::system_clock::time_point _start_time;

	public :

	    timer ();

	    /**
	     * restart the timer
	     */
	    void reset (std::chrono::system_clock::time_point, std::chrono::duration<double> since) ;

	    /**
	     * restart the timer
	     */
	    void reset () ;

	    /**
	     * @return: the number of seconds since last start
	     */
	    float time_since_start () const;

	    /**
	     * Sleep for nbSec
	     * @returns: the number of second left (if interrupted by a signal for example)
	     */
	    static float sleep (float nbSec);

		/**
		 * @returns: the number of second since epoch
		 */
		static unsigned long time_since_epoch ();

	};
	
    }

}
