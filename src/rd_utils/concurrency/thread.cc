#include <rd_utils/concurrency/thread.hh>

namespace rd_utils {

    namespace concurrency {

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
				closure (nullptr),
				func (nullptr),
				content (0, nullptr)
			{}

			dg_thread_launcher::dg_thread_launcher (fake* closure, void (fake::*func) (Thread)) :
				closure (closure),
				func (func),
				content (0, new ThreadPipe (true))
			{}

			void dg_thread_launcher::run () {
				(this-> closure->* (this-> func)) (this-> content);
			}

			void dg_thread_launcher::dispose () {
				delete this-> content.pipe;
			}

			fn_thread_launcher::fn_thread_launcher () :
				func (nullptr),
				content (0, nullptr)
			{}

			fn_thread_launcher::fn_thread_launcher (void (*func) (Thread)) :
				func (func),
				content (0, new ThreadPipe (true))
			{}

			void fn_thread_launcher::run () {
				this-> func (this-> content);
			}

			void fn_thread_launcher::dispose () {
				delete this-> content.pipe;
			}

		}

    }

}
