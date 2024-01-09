#pragma once

#include "signal.hh"
#include "thread.hh"
#include "mutex.hh"
#include "mailbox.hh"
#include <unordered_map>

namespace rd_utils::concurrency {

  class Task {
  public:
    virtual void execute (Thread) = 0;
  };

  namespace internal_pool {

    class fake {};

    /**
     * ===============================================================
     * ===============================================================
     * ===================         FUNCTIONS       ===================
     * ===============================================================
     * ===============================================================
     */

    class fn_task : public Task {
    public:
      void (*func) ();

      fn_task (void (*func) ());
      void execute (Thread) override;
    };

    template <typename ... T>
    class fn_task_template : public Task {
    public:
      void (*func) (T...);
      std::tuple<T...> datas;

      fn_task_template (void (*func) (T...), T... args) :
        func (func),
        datas (std::make_tuple (args...))
      {}

      void execute (Thread) override {
        std::apply ([this](auto&& ... args) {
          this-> func (args...);
        }, this-> datas);
      }

    };

    /**
     * ===============================================================
     * ===============================================================
     * ===================         DELEGATES       ===================
     * ===============================================================
     * ===============================================================
     */


    class dg_task : public Task {
    public:
      fake* closure;
      void (internal_pool::fake::*func) ();

      dg_task (fake * closure, void (internal_pool::fake::*func) ());
      void execute (Thread) override;
    };

    template <typename ... T>
    class dg_task_template : public Task {
      fake* closure;
      void (internal_pool::fake::*func) (T...);
      std::tuple<T...> datas;
    public:

      dg_task_template (fake * closure, void (internal_pool::fake::*func) (T...), T ... args) :
        closure (closure),
        func (func),
        datas (std::make_tuple (args...))
      {}

      void execute (Thread) override {
        std::apply ([this](auto &&... args) {
          (this-> closure->* (this-> func)) (args...);
        }, this-> datas);
      }

    };

  }

  class TaskPool {
  private:

    // True if the taskpool started
    bool _terminated = false;

    // The list of jobs to launch
    Mailbox <Task*> _jobs;

    // The number of task that were submitted
    unsigned int _nbSubmitted = 0;

    // The number of task that were completed
    unsigned int _nbCompleted = 0;

    // The maximum number of thread that can be launched by the task pool
    unsigned int _nbThread = 0;

    // The list of running threads
    std::unordered_map<unsigned int, Thread> _runningThreads;

    // The list of closed threads
    Mailbox<Thread> _closed;

    // The mutex of the task pool
    mutex _m;

    // Mutex to wait a new task
    mutex _waitTask;

    // Signal emitted when a new task is submitted
    condition _waitTaskSig;

    // Mutex to wait for a task to be completed
    mutex _completeTask;

    // Signal emitted when a task is completed
    condition _completeTaskSig;

    // Mutex locked when waiting for a thread to be ready
    mutex _ready;

    // Signal emitted by a thread when ready
    condition _readySig;

  private :

    TaskPool (const TaskPool & other);
    void operator= (const TaskPool & other);

  public :

    /**
     * Create a new task pool
     * @params:
     *    - nbThreads: the maximum number of thread that can run concurrently in the pool
     */
    TaskPool (unsigned int nbThreads);

    /**
     * Move ctor
     */
    TaskPool (TaskPool && other);

    /**
     * Move affectation
     */
    void operator=(TaskPool && other);

    /**
     * Submit a delegate task
     */
    template <class X>
    void submit (X * x, void (X::*func) ()) {
      this-> submit (new internal_pool::dg_task ((internal_pool::fake*) x, (void (internal_pool::fake::*) ()) func));
    }

    /**
     * Submit a task with parameters
     */
    template <typename ... T>
    void submit (void (*func)(T...), T... args) {
      this-> submit (new internal_pool::fn_task_template (func, args...));
    }

    template <class X, typename ... T>
    void submit (X * x, void (X::*func) (T...), T... args) {
      this-> submit (new internal_pool::dg_task_template ((internal_pool::fake*) x, (void (internal_pool::fake::*)(T...)) func, args...));
    }

    /**
     * Submit a function task
     */
    void submit (void (*func) ());

        /**
     * Submit a function task
     */
    void submit (void (*func) (Thread));

    /**
     * Wait for all the submitted task to end
     */
    void join ();

  private:

    /**
     * Submit a new task in the pool
     */
    void submit (Task * task);

    /**
     * Wait for all the tasks to be completed
     */
    void waitAllCompletes ();

    /**
     * Spawn the threads in the pool
     * */
    void spawnThread ();

    /**
     * The function of a running task
     */
    void taskMain (Thread);

  };


}
