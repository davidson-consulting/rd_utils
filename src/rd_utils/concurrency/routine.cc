#include "routine.hh"
#include <rd_utils/utils/error.hh>

namespace rd_utils::concurrency {


	Routine::Routine (uint64_t freq)
		: _freq (freq)
		, _task (nullptr)
		, _sem (std::make_shared <concurrency::semaphore> ())
		, _th (0, nullptr)
	{}

	Routine& Routine::start (void (*func) ()) {
		this-> start (std::make_shared <internal_pool::fn_task> (func));
		return *this;
	}

	void Routine::start (std::shared_ptr <Task> task) {
		if (this-> _isRunning) throw utils::Rd_UtilsError ("Routine already started");

		this-> _task = task;
		this-> _isRunning = true;
		this-> _th = concurrency::spawn (this, &Routine::taskMain);
	}

	void Routine::stop () {
		this-> _isRunning = false;
		this-> _sem-> post ();
	}

	bool Routine::join () {
		if (this-> _th.id == 0) return false;

		pthread_t self = concurrency::Thread::self ();
		if (this-> _th.equals (self)) return false;

		concurrency::join (this-> _th);
		this-> _th = Thread (0, nullptr);

		return true;
	}

	void Routine::taskMain (concurrency::Thread) {
		while (this-> _isRunning) {
			this-> _t.reset ();
			this-> _task-> execute (Thread (0, nullptr));

			auto took = this-> _t.time_since_start ();
			auto suppose = 1.0f / this-> _freq;
			if (took < suppose) {
				if (this-> _sem-> wait (suppose - took)) {
					return;
				}
			}
		}
	}

}
