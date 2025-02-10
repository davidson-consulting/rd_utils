#pragma once

#include <pthread.h>
#include <tuple>
#include "tpipe.hh"

namespace rd_utils {
    namespace concurrency {

		struct Thread {
			pthread_t id;

			Thread (int id) :
				id (id)
			{}

			/**
			 * @returns: true if the thread is equal to other
			 */
			bool equals (pthread_t) const;

			/**
			 * @returns: true if the thread is the currently running thread
			 */
			bool isSelf () const;

			/**
			 * @returns: the self thread (current thread id)
			 */
			static pthread_t self ();

		};

		namespace internal {
			void* thread_fn_main (void * inner);
			void* thread_dg_main (void * inner);

			class fake {};

			class dg_thread_launcher {
			protected:

				dg_thread_launcher ();

			public:
				Thread content;
				fake* closure;
				void (fake::*func) (Thread);

				dg_thread_launcher (fake* closure, void (fake::*func) (Thread));

				virtual void run ();

				virtual void dispose ();

				virtual ~dg_thread_launcher ();
			};

			class fn_thread_launcher {
			protected:

				fn_thread_launcher ();

			public:
				Thread content;
				void (*func) (Thread);

				fn_thread_launcher (void (*func) (Thread));

				virtual void run ();

				virtual void dispose ();

				virtual ~fn_thread_launcher ();
			};

			template <typename ... T>
			class dg_thread_launcher_template : public dg_thread_launcher {
			public:
				Thread content;
				fake * closure;
				std::tuple <T...> datas;
				void (fake::*func) (Thread, T...);

				dg_thread_launcher_template (fake* closure, void (fake::*func) (Thread, T...), T... args) :
					dg_thread_launcher (),
					content (0),
					closure (closure), func (func), datas (std::make_tuple (args...))
				{}

				void run () override  {
					std::apply ([this](auto &&... args) {
						(this-> closure->* (this-> func)) (this-> content, args...);
					}, this-> datas);

				}

				void dispose () {}
			};

			template <typename ... T>
			class fn_thread_launcher_template : public fn_thread_launcher {
			public:
				Thread content;
				void (*func) (Thread, T...);
				std::tuple <T...> datas;

				fn_thread_launcher_template (void (*func) (Thread, T...), T... args) :
					fn_thread_launcher (),
					content (0), func (func), datas (std::make_tuple (args...))
				{}

				void run () override {
					std::apply (this-> func, std::tuple_cat (std::make_tuple (this-> content), this-> datas));
				}

				void dispose () {}

			};
		}

		template <typename ... T>
		Thread spawn (void (*func) (Thread, T...), T... args) {
			auto th = new internal::fn_thread_launcher_template<T...> (func, args...);
			pthread_create (&th-> content.id, nullptr, &internal::thread_fn_main, th);
			return th-> content;
		}

		/**
		 * Spawn a new thread that will run a method
		 * @params:
		 *  - func: the main method of the thread
		 */
		template <class X>
		Thread spawn (X * x, void (X::*func)(Thread)) {
			auto th = new internal::dg_thread_launcher ((internal::fake*) x, (void (internal::fake::*)(Thread)) func);
			pthread_create (&th-> content.id, nullptr, &internal::thread_dg_main, th);
			return th-> content;
		}


		template <class X, typename ... T>
		Thread spawn (X * x, void (X::*func)(Thread, T...), T... args) {
			auto th = new internal::dg_thread_launcher_template ((internal::fake*)x, (void (internal::fake::*)(Thread, T...)) func, args...);
			pthread_create (&th-> content.id, nullptr, &internal::thread_dg_main, th);
			return th-> content;
		}

		/**
		 * Wait the end of the execution of the thread
		 * @params:
		 *  - th: the thread to wait
		 */
		void join (Thread th);

    }

}
