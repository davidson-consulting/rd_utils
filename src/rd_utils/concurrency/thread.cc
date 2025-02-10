#include <rd_utils/concurrency/thread.hh>

namespace rd_utils {

    namespace concurrency {

		bool Thread::equals (pthread_t other) const {
			return this-> id == other;
		}

		bool Thread::isSelf () const {
			return this-> id == Thread::self ();
		}

		pthread_t Thread::self () {
			return pthread_self ();
		}

		Thread spawn (void (*func)(Thread)) {
			auto th = new internal::fn_thread_launcher (func);
			pthread_create (&th-> content.id, nullptr, &internal::thread_fn_main, th);
			return th-> content;
		}

		void join (Thread th) {
			pthread_join (th.id, nullptr);
		}


		namespace internal {
			void* thread_fn_main (void * inner) {
				fn_thread_launcher * fn = (fn_thread_launcher*)(inner);
				fn-> run ();
				fn-> dispose ();
				delete (fn_thread_launcher*) inner;
				return nullptr;
			}

			void* thread_dg_main (void * inner) {
				dg_thread_launcher * dg = (dg_thread_launcher*)(inner);
				dg-> run ();
				dg-> dispose ();
				delete (dg_thread_launcher*) inner;
				return nullptr;
			}

			dg_thread_launcher::dg_thread_launcher () :
				content (0),
				closure (nullptr),
				func (nullptr)
			{}

			dg_thread_launcher::dg_thread_launcher (fake* closure, void (fake::*func) (Thread)) :
				content (0),
				closure (closure),
				func (func)
			{}

			void dg_thread_launcher::run () {
				(this-> closure->* (this-> func)) (this-> content);
			}

			void dg_thread_launcher::dispose () {}

			dg_thread_launcher::~dg_thread_launcher () {
				this-> dispose ();
			}

			fn_thread_launcher::fn_thread_launcher () :
				content (0),
				func (nullptr)
			{}

			fn_thread_launcher::fn_thread_launcher (void (*func) (Thread)) :
				content (0),
				func (func)
			{}

			void fn_thread_launcher::run () {
				this-> func (this-> content);
			}

			void fn_thread_launcher::dispose () {}

			fn_thread_launcher::~fn_thread_launcher () {
				this-> dispose ();
			}

		}

    }

}
