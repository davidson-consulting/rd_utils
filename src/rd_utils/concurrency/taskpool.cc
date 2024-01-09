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

  void TaskPool::waitAllCompletes () {
    while (this-> _nbSubmitted != this-> _nbCompleted) {
      WITH_LOCK (this-> _completeTask) {
        this-> _completeTaskSig.wait (this-> _completeTask);
      }
    }
  }

  void TaskPool::join () {
    this-> waitAllCompletes ();
    this-> _terminated = true;

    while (this-> _closed.len () != this-> _runningThreads.size ()) {
      this-> _waitTaskSig.signal ();
    }

    for (auto [i, th] : this-> _runningThreads) {
      concurrency::join (th);
    }

    this-> _closed.clear ();
    this-> _runningThreads.clear ();
    this-> _nbCompleted = 0;
    this-> _nbSubmitted = 0;
  }

  void TaskPool::submit (Task * task) {
    this-> _jobs.send (task);
    if (this-> _runningThreads.size () != this-> _nbThread) {
      this-> spawnThread ();
    }

    this-> _nbSubmitted += 1;
    this-> _waitTaskSig.signal ();
  }

  void TaskPool::spawnThread () {
    this-> _terminated = false;
    for (int i = 0 ; i < this-> _nbThread ; i++) {
      auto th = spawn (this, &TaskPool::taskMain);
      WITH_LOCK (this-> _ready) {
        this-> _readySig.wait (this-> _ready);
        this-> _runningThreads.emplace (th.id, th);
      }
    }
  }

  void TaskPool::taskMain (Thread t) {
    this-> _readySig.signal ();
    for (;;) {
      WITH_LOCK (this-> _waitTask) {
        this-> _waitTaskSig.wait (this-> _waitTask);
      }

      // Signal was emitted to kill the thread
      if (this-> _terminated) break;

      // Otherwise a new task is ready
      for (;;) { // maybe multiple tasks are in the list
        auto msg = this-> _jobs.receive ();
        if (msg.has_value ()) {
          Task * task = *msg;
          task-> execute (t);
          delete task;

          WITH_LOCK (this-> _m) { // Task completed
            this-> _nbCompleted += 1;
            this-> _completeTaskSig.signal ();
          }
        } else { // No task found
          break;
        }
      }
    }

    this-> _closed.send (t); // Thread is exiting
  }


}
