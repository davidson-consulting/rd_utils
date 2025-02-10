#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <rd_utils/net/stream.hh>
#include <rd_utils/net/server.hh>
#include <rd_utils/concurrency/mutex.hh>
#include <rd_utils/concurrency/mailbox.hh>
#include <rd_utils/utils/_.hh>
#include <rd_utils/memory/cache/_.hh>

namespace rd_utils::concurrency::actor {

  class ActorRef;
  class ActorBase;

  enum class ActorSystemLimits : int32_t {
    BASE_TIMEOUT = 5
  };

  /**
   * Actor system class used to register actors, and manage communications
   */
  class ActorSystem {
  public:

    struct Response {
      uint64_t reqId;
      std::shared_ptr <rd_utils::utils::config::ConfigNode> msg;
    };

  private:

    struct Job {
      uint32_t protId;
      std::shared_ptr <net::TcpStream> stream;
    };


    // If true the server should be killed when all actors are killed
    bool _stopOnEmpty = false;

    // The list of local actors
    std::map <std::string, std::shared_ptr <ActorBase> > _actors;

    // The mutex locked when manupulating actor collection
    concurrency::mutex _actMut;

    // The mutex locked when manupulating request ids
    concurrency::mutex _rqMut;

    // Mutex to manage the _conn collection
    concurrency::mutex _connM;

    // True while the system is running
    bool _isRunning = false;

  private:

    // The epoll list
    int _epoll_fd = 0;

    net::SockAddrV4 _addr;
    
    // The context of the queuing
    std::shared_ptr <net::TcpListener> _listener;

    // The number of threads in the pool
    uint32_t _nbJobThreads;
    uint32_t _nbManageThreads;

    // The polling thread
    concurrency::Thread _th;

    // The worker threads
    std::map <int, concurrency::Thread> _runningJobThreads;
    std::map <int, concurrency::Thread> _runningManageThreads;
    concurrency::Thread _treatTh;
    
    concurrency::Mailbox <Job> _jobs;
    concurrency::Mailbox <Job> _manages;
    concurrency::Mailbox <std::shared_ptr <net::TcpStream> > _treat;


    concurrency::ThreadPipe _trigger;
    concurrency::mutex _triggerM;
    concurrency::semaphore _ready;
    concurrency::semaphore _waitJobTask;
    concurrency::semaphore _waitManageTask;
    concurrency::semaphore _waitTreatTask;
    
  private:

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          REQUESTS          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    // mailbox of responses
    std::map <uint64_t, Response> _responses;

    // Request being resolved but that might timeout
    std::map <uint64_t, std::shared_ptr <semaphore> > _requestIds;

    // Uniq id
    uint64_t _lastU = 0;

  public:

    enum class Protocol : int {
      ACTOR_EXIST_REQ,
      ACTOR_MSG,
      ACTOR_REQ,
      ACTOR_REQ_BIG,
      ACTOR_RESP,
      ACTOR_RESP_BIG,
      SYSTEM_CLOSED,
      SYSTEM_KILL_ALL
    };

  public:

    /**
     * @params:
     *    - addr: the accepted by the actor system
     *    - nbThreads: the number of threads used (-1 means nb cores available on system)
     *    - nbManageThreads: the number of threads used to the management
     */
    ActorSystem (net::SockAddrV4 addr, int nbThreads = -1, int nbManageThreads = 1);

    /**
     * Stop the system when all the actors are exited after start
     */
    ActorSystem& joinOnEmpty (bool join);

    /**
     * Start the actor system
     */
    void start ();

    /**
     * Register an actor
     * @params:
     *    - name: the name of the actor
     *    - actor: the actor to add
     */
    template <typename T>
    void add (const std::string & name) {
      WITH_LOCK (this-> _actMut) {
        auto it = this-> _actors.find (name);
        if (it != this-> _actors.end ()) {
          LOG_ERROR ("Already an actor named '" + name + "'");
          throw std::runtime_error ("Already an actor named '" + name + "'");
        }
      }

      try {
        auto act = std::make_shared <T> (name, this);
        WITH_LOCK (this-> _actMut) {
          this-> _actors.emplace (name, act);
        }

        act-> onStart ();
      } catch (...) {
        LOG_ERROR ("Actor '", name, "' crashed at startup");
        WITH_LOCK (this-> _actMut) {
          this-> _actors.erase (name);
        }
        throw std::runtime_error ("Actor '" + name + "' crashed at startup");
      }
    }

    /**
     * Register an actor
     * @params:
     *    - name: the name of the actor
     *    - actor: the actor to add
     */
    template <typename T, typename ... Args>
    void add (const std::string & name, Args... a) {
      WITH_LOCK (this-> _actMut) {
        auto it = this-> _actors.find (name);
        if (it != this-> _actors.end ()) {
          LOG_ERROR ("Already an actor named '" + name + "'");
          throw std::runtime_error ("Already an actor named '" + name + "'");
        }
      }

      try {
        auto act = std::make_shared <T> (name, this, a...);
        WITH_LOCK (this-> _actMut) {
          this-> _actors.emplace (name, act);
        }

        act-> onStart ();
      } catch (...) {
        LOG_ERROR ("Actor '", name, "' crashed at startup");
        WITH_LOCK (this-> _actMut) {
          this-> _actors.erase (name);
        }

        throw std::runtime_error ("Actor '" + name + "' crashed at startup");
      }
    }

    /**
     * Remove an actor
     * @params:
     *    - name: the name of the actor to remove
     *    - lock: true if it is not done from the inside of the actor
     * @info: does nothing if the actor does not exist
     */
    void remove (const std::string & name, bool lock = true);

    /**
     * @returns: an actor reference to a local actor
     */
    std::shared_ptr<ActorRef> localActor (const std::string & name);

    /**
     * @returns: an actor reference to a remote actor
     */
    std::shared_ptr<ActorRef> remoteActor (const std::string & name, net::SockAddrV4 addr);

    /**
     * Close the actor system and all registered actors
     * @returns: true if the dispose was successful
     * @info: only an external thread can dispose the system (not a thread spawned by the system itself)
     */
    bool dispose ();

    /**
     * Wait for the actor system to be disposed
     * @returns: true if the join was successful
     * @info: only an external thread can join the system (not a thread spawned by the system itself)
     */
    bool join ();

    /**
     * @returns: the port of the listening server
     */
    uint32_t port ();

    /**
     * Clean
     */
    ~ActorSystem ();


  private:

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          TASK POOL          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */


    /**
     * The main loop of the actor system accepting connection
     * This loop submits the connection to the task pool to be executed by the actors
     */
    void pollMain (concurrency::Thread);

    /**
     * Configure the epoll
     */
    void configureEpoll ();

    /**
     * Submit a new client to the task pool
     */
    void submitJob (uint32_t, std::shared_ptr <net::TcpStream> stream);
    void submitManage (uint32_t, std::shared_ptr <net::TcpStream> stream);


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          WORKERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    /**
     * Spawn the threads
     */
    void spawnJobThreads ();
    void spawnManageThreads ();

    /**
     * The main loop of a worker thread
     */
    void workerJobThread (concurrency::Thread);
    void workerManageThread (concurrency::Thread);
    void workerTreatThread (concurrency::Thread);    

    /**
     * Called when a tcpstream message is received
     * @params:
     *    - stream: the stream to the tcp client
     */
    void onSession (uint32_t, std::shared_ptr <net::TcpStream> stream);
    void onManage (uint32_t, std::shared_ptr <net::TcpStream> stream);

    /**
     * @returns: true iif this is a worker thread
     */
    bool isWorkerThread () const;

  private:

    friend ActorRef;

    void poisonPill ();

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          RESPONSES          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    /**
     * Push a response
     */
    void pushResponse (Response && rep);

    /**
     * generate a uniq id
     */
    uint64_t genUniqId ();

    /**
     * Register the fact that an actor is waiting for the request to be resolved
     */
    void registerRequestId (uint64_t id, std::shared_ptr <semaphore> who);

    /**
     * Register the fact that the actor waiting for the request ceise to wait
     */
    void removeRequestId (uint64_t id);

    /**
     * Consume the response if it exists
     */
    bool consumeResponse (uint64_t id, Response & resp);

    /**
     * Treat an actor existance request
     */
    void onActorExistReq (std::shared_ptr <net::TcpStream> session);

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          MESSAGES          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    /**
     * Treat an actor message
     */
    void onActorMsg (std::shared_ptr <net::TcpStream> session);

    /**
     * Treat an actor request (msg with response)
     */
    void onActorReq (std::shared_ptr <net::TcpStream> session);

    /**
     * Treat an actor response
     */
    void onActorResp (std::shared_ptr <net::TcpStream> session);

    /**
     * Treat a request to kill the system
     */
    void onSystemKill ();

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    /**
     * @returns: the name of the actor
     */
    std::string readActorName (std::shared_ptr <net::TcpStream> stream);

    /**
     * @returns: read an address from remote stream
     */
    net::SockAddrV4 readAddress (std::shared_ptr <net::TcpStream> stream);

    /**
     * @returns: a message
     */
    std::shared_ptr<utils::config::ConfigNode> readMessage (std::shared_ptr <net::TcpStream> stream);


    /**
     * @returns: an actor in the system
     */
    bool getActor (const std::string & name, std::shared_ptr <ActorBase>& act);

    /**
     * Remove an actor from the system
     */
    void removeActor (const std::string & name);


    void waitAllCompletes ();

  };

}
