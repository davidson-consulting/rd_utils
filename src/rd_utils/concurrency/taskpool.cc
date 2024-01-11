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


  TaskPool::TaskPool (TaskPool && other) :
    _terminated (other._terminated)
    , _jobs (std::move (other._jobs))
    , _nbSubmitted (other._nbSubmitted)
    , _nbCompleted (other._nbCompleted)
    , _nbThread (other._nbThread)
    , _runningThreads (std::move (other._runningThreads))
    , _closed (std::move (other._closed))
    , _m (std::move (other._m))
    , _waitTask (std::move (other._waitTask))
    , _completeTask (std::move (other._completeTask))
    , _ready (std::move (other._ready))
  {
    other._nbSubmitted = 0;
    other._nbCompleted = 0;
  }

  void TaskPool::operator=(TaskPool && other) {
    this-> join ();
    this-> _terminated = other._terminated;
    this-> _jobs = std::move (other._jobs);
    this-> _nbSubmitted = other._nbSubmitted;
    this-> _nbCompleted = other._nbCompleted;
    this-> _nbThread = other._nbThread;
    this-> _runningThreads = std::move (other._runningThreads);
    this-> _closed = std::move (other._closed);
    this-> _m = std::move (other._m);
    this-> _waitTask = std::move (other._waitTask);
    this-> _completeTask = std::move (other._completeTask);
    this-> _ready = std::move (other._ready);

    other._nbSubmitted = 0;
    other._nbCompleted = 0;
  }

  void TaskPool::submit (void (*func) ()) {
    this-> submit (new internal_pool::fn_task (func));
  }

  void TaskPool::waitAllCompletes () {
    while (this-> _nbSubmitted != this-> _nbCompleted) {
      this-> _completeTask.wait ();
    }
  }

  void TaskPool::join () {
    this-> waitAllCompletes ();
    this-> _terminated = true;

    while (this-> _closed.len () != this-> _runningThreads.size ()) {
      this-> _waitTask.post ();
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
    this-> _waitTask.post ();
  }

  void TaskPool::spawnThread () {
    this-> _terminated = false;
    for (int i = 0 ; i < this-> _nbThread ; i++) {
      auto th = spawn (this, &TaskPool::taskMain);
      this-> _ready.wait ();
      this-> _runningThreads.emplace (th.id, th);
    }
  }

  void TaskPool::taskMain (Thread t) {
    this-> _ready.post ();
    for (;;) {
      this-> _waitTask.wait ();

      // Signal was emitted to kill the thread
      if (this-> _terminated) break;

      // Otherwise a new task is ready
      for (;;) { // maybe multiple tasks are in the list
        Task * task = nullptr;
        if (this-> _jobs.receive (task)) {
          task-> execute (t);
          delete task;

          WITH_LOCK (this-> _m) { // Task completed
            this-> _nbCompleted += 1;
            this-> _completeTask.post ();
          }
        } else { // No task found
          break;
        }
      }
    }

    this-> _closed.send (t); // Thread is exiting
  }


}
