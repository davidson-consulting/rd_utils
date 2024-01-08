#include "taskpool.hh"

#include <unistd.h>
#include <rd_utils/utils/log.hh>

namespace rd_utils::concurrency {

  namespace internal_pool {
    fn_task::fn_task (void (*func) ()) : func (func) {}

    void fn_task::execute (Thread) {
      this-> func ();
    }

    dg_task::dg_task (fake * closure, void (fake::*func) ()) :
      closure (closure),
      func (func)
    {}

    void dg_task::execute (Thread) {
      (this-> closure->* (this-> func)) ();
    }

  }

  TaskPool::TaskPool (unsigned int nbThreads)
    : _nbThread (nbThreads)
  {}


  void TaskPool::submit (void (*func) ()) {
    this-> submit (new internal_pool::fn_task (func));
  }

  void TaskPool::join () {
    for (auto [i, th] : this-> _runningThreads) {
      concurrency::join (th);
    }

    this-> _runningThreads.clear ();
  }

  void TaskPool::submit (Task * task) {
    this-> _jobs.send (task);
    this-> removeExitedThreads ();
    int jobLen = this-> _jobs.len () + 1;
    int max_th = this-> _nbThread - this-> _runningThreads.size ();
    if (max_th != 0) {
      if (max_th > jobLen) {
        this-> spawnThread (jobLen, this-> _runningThreads.size ());
      } else {
        this-> spawnThread (max_th, this-> _runningThreads.size ());
      }
    }
  }

  void TaskPool::spawnThread (unsigned int nb, unsigned int id) {
    std::cout << "Spawning nb " << nb << std::endl;
    for (int i = 0 ; i < nb ; i++) {
      auto th = spawn (this, &TaskPool::taskMain, id + i);
      WITH_LOCK (this-> _m) {
        this-> _runningThreads.emplace (id + i, th);
      }
    }
  }

  void TaskPool::taskMain (Thread t, unsigned int id) {
#define EXIT_THREAD_AFTER_LOST 100
    int nbSkips = 0;
    while (true) {
      auto msg = this-> _jobs.receive ();
      if (msg.has_value ()) {
        Task * task = *msg;
        task-> execute (t);
        delete task;
      } else {
        nbSkips += 1;
        if (nbSkips > EXIT_THREAD_AFTER_LOST) {
          break;
        }

        usleep (1000);
      }
    }

    this-> _exited.send (id);
  }

  void TaskPool::removeExitedThreads () {
    while (true) {
      auto msg = this-> _exited.receive ();
      if (msg.has_value ()) {
        auto th = this-> _runningThreads.find (*msg);
        if (th != this-> _runningThreads.end ()) {
          concurrency::join (th-> second);
          WITH_LOCK (this-> _m) {
            this-> _runningThreads.erase (*msg);
          }
        }
      } else {
        break;
      }
    }
  }


}
