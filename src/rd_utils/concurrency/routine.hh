#pragma once

#include <rd_utils/concurrency/thread.hh>
#include <rd_utils/concurrency/timer.hh>
#include <rd_utils/concurrency/taskpool.hh>
#include <cstdint>
#include <memory>

namespace rd_utils::concurrency {

	/**
	 * A thread routine executing a function at a defined interval
	 *  */
	class Routine {
	private:

		// The frequency of the routine
		uint64_t _freq;

		// The timer of the routine
		concurrency::timer _t;

		// THe routine to call
		std::shared_ptr <Task> _task;

		// Semaphore used to stop the routine
		std::shared_ptr <concurrency::semaphore> _sem;

		// True iif the routine is running
		bool _isRunning = false;

		// The running thread
		concurrency::Thread _th;

	public:

		/**
		 * @params:
		 *    - freq: The frequency of the routine (number of time of execution by second)
		 *  */
		Routine (uint64_t freq);



		/**
		 * Start the routine with a function
		 */
		Routine& start (void (*func) ());

		/**
		 * Start the routine with a function pointer
		 */
		template <typename ... T>
		Routine& start (void (*func)(T...), T... args) {
			this-> start (std::make_shared <internal_pool::fn_task_template> (func, args...));
			return *this;
		}

		/**
		 * Start the routine with a delegate
		 */
		template <class X>
		Routine& start (X * x, void (X::*func) ()) {
			this-> start (std::make_shared <internal_pool::dg_task> ((internal_pool::fake*) x, (void (internal_pool::fake::*) ()) func));
			return *this;
		}

		/**
		 * Start the routine with a delegate
		 */
		template <class X, typename ... T>
		Routine& start (X * x, void (X::*func) (T...), T... args) {
			this-> start (std::make_shared <internal_pool::dg_task_template> ((internal_pool::fake*) x, (void (internal_pool::fake::*)(T...)) func, args...));
			return *this;
		}

		/**
		 * Stop the routine
		 *  */
		void stop ();

		/**
		 * Wait for the routine to complete
		 *  */
		bool join ();

	private:

		/**
		 * Set the task of the routine
		 */
		void start (std::shared_ptr <Task> task);

		/**
		 * The routine
		 *  */
		void taskMain (Thread);


	};

}
