#pragma once

#include <pthread.h>

namespace rd_utils {

    namespace concurrency {

		class mutex_locker;
		class condition;


		class mutex {

			pthread_mutex_t _m;

		public:
	    
			mutex ();


			void lock ();

			void unlock ();


			~mutex ();

		private:

			friend mutex_locker;
			friend condition;

		};

		class mutex_locker {

			mutex & _m;

		public:

			mutex_locker (mutex & m);

			~mutex_locker ();

		};
	
    }    

}


#define WITH_LOCK(m)												\
	if (auto m_ = rd_utils::concurrency::mutex_locker (m)  ; true)
