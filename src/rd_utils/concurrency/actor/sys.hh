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
  class ActorMessage;

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
      std::vector <uint8_t> data;
      net::Ipv4Address origin;
    };

    // If true the server should be killed when all actors are killed
    bool _stopOnEmpty = false;

    // The list of local actors
    std::map <std::string, std::shared_ptr <ActorBase> > _actors;

    // The mutex locked when manupulating actor collection
    concurrency::mutex _actMut;

    // The mutex locked when manupulating request ids
    concurrency::mutex _rqMut;

    // True while the system is running
    bool _isRunning = false;

  private:

    // The epoll list
    int _epoll_fd = 0;

    // The list of connected clients
    std::unordered_map <uint32_t, std::shared_ptr <net::TcpStream> > _sockets;
    std::unordered_map <uint64_t, std::list <std::shared_ptr <net::TcpStream> > > _remotes;

    net::SockAddrV4 _addr;
    
    // The context of the queuing
    std::shared_ptr <net::TcpListener> _listener;

    // The number of threads in the pool
    uint32_t _nbJobThreads;

    // The polling thread
    concurrency::Thread _th;

    // The worker threads
    std::map <int, concurrency::Thread> _runningJobThreads;
    concurrency::Mailbox <Job> _jobs;

    concurrency::ThreadPipe _trigger;
    concurrency::mutex _triggerM;
    concurrency::semaphore _ready;
    concurrency::semaphore _waitJobTask;
    
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


  public:

    /**
     * @params:
     *    - addr: the accepted by the actor system
     *    - nbThreads: the number of threads used (-1 means nb cores available on system)
     */
    ActorSystem (net::SockAddrV4 addr, int nbThreads = -1);

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
    uint32_t port () const;

    /**
     * @returns: the address of the system
     */
    net::SockAddrV4 addr () const;

    /**
     * generate a uniq id for messages
     */
    uint64_t genUniqId ();

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
    void submitJob (Job && msg);

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

    /**
     * The main loop of a worker thread
     */
    void workerJobThread (concurrency::Thread);

    /**
     * Called when a tcpstream message is received
     * @params:
     *    - stream: the stream to the tcp client
     */
    void onSession (const ActorMessage&);

    /**
     * @returns: true iif this is a worker thread
     */
    bool isWorkerThread () const;

  private:

    friend ActorRef;

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
    void onActorMsg (const ActorMessage & msg);

    /**
     * Treat an actor request (msg with response)
     */
    void onActorReq (const ActorMessage & msg);

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
     * Add a socket to the list of connected sockets
     */
    void addPoll (std::shared_ptr <net::TcpStream> socket);

    /**
     * Delete a socket to the list of connected sockets
     */
    void delPoll (uint32_t fd);

    /**
     * @returns:  true if this is the address of the local actor system
     */
    bool isLocalAddr (net::SockAddrV4) const;

    /**
     * Read a message from a remote system and submit it to the correct
     */
    void receiveMessage (std::shared_ptr <net::TcpStream> stream);

    /**
     * Send a message to a remote actor system
     */
    bool sendMessage (net::SockAddrV4 addr, const ActorMessage & msg, uint32_t nbRetries, bool isLocal = false);

    /**
     * @returns: an actor in the system
     */
    bool getActor (const std::string & name, std::shared_ptr <ActorBase>& act);

    /**
     * Remove an actor from the system
     */
    void removeActor (const std::string & name);


    void waitAllCompletes ();


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ================================          CONNECTION POOL          =================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::shared_ptr <net::TcpStream> connect (net::SockAddrV4 addr);
    void release (net::SockAddrV4 addr, std::shared_ptr <net::TcpStream> conn);

  };

}
