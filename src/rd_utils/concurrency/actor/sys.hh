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
  class ActorStream;

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

    struct ResponseBig {
      uint64_t reqId;
      std::shared_ptr <rd_utils::memory::cache::collection::CacheArrayBase> msg;
    };

    struct ResponseStream {
      uint64_t reqId;
      std::shared_ptr <net::TcpSession> stream;
    };

  private:


    // The server used to communicate between actors
    net::TcpServer _server;

    // Number of threads the actor system can manage at the same time
    uint64_t _nbThreads;

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

    // mailbox of responses
    std::map <uint64_t, Response> _responses;

    // mailbox of big responses
    std::map <uint64_t, ResponseBig> _responseBigs;

    // Streaming response
    std::map <uint64_t, ResponseStream> _responseStreams;

    // Request being resolved but that might timeout
    std::map <uint64_t, std::shared_ptr <semaphore> > _requestIds;

    // Uniq id
    uint64_t _lastU = 0;

    bool _stopOnEmpty = false;

  public:

    enum class Protocol : int {
      ACTOR_EXIST_REQ,
      ACTOR_MSG,
      ACTOR_REQ,
      ACTOR_REQ_BIG,
      ACTOR_RESP,
      ACTOR_RESP_BIG,
      ACTOR_REQ_STREAM,
      ACTOR_RESP_STREAM,
      SYSTEM_CLOSED,
      SYSTEM_KILL_ALL
    };

  public:

    /**
     * @params:
     *    - addr: the accepted by the actor system
     *    - nbThreads: the number of threads used (-1 means nb cores available on system)
     */
    ActorSystem (net::SockAddrV4 addr, int nbThreads = -1, int maxCon = -1);

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
      auto it = this-> _actors.find (name);
      if (it != this-> _actors.end ()) throw std::runtime_error ("Already an actor named : " + name);

      auto act = std::make_shared <T> (name, this);

      WITH_LOCK (this-> _actMut) {
        this-> _actors.emplace (name, act);
      }

      act-> onStart ();
    }

    /**
     * Register an actor
     * @params:
     *    - name: the name of the actor
     *    - actor: the actor to add
     */
    template <typename T, typename ... Args>
    void add (const std::string & name, Args... a) {
      auto it = this-> _actors.find (name);
      if (it != this-> _actors.end ()) throw std::runtime_error ("Already an actor named : " + name);

      auto act = std::make_shared <T> (name, this, a...);

      WITH_LOCK (this-> _actMut) {
        this-> _actors.emplace (name, act);
      }

      act-> onStart ();
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
     */
    void dispose ();

    /**
     * Wait for the actor system to be disposed
     */
    void join ();

    /**
     * @returns: the port of the listening server
     */
    uint32_t port ();

    /**
     * Clean
     */
    ~ActorSystem ();


  private:

    friend ActorRef;

    void poisonPill ();

    /**
     * Push a response
     */
    void pushResponse (Response rep);

    /**
     * Push a big response
     */
    void pushResponseBig (ResponseBig rep);

    /**
     * Push a stream response
     */
    void pushResponseStream (ResponseStream rep);

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
     * Consume the response if it exists
     */
    bool consumeResponseBig (uint64_t id, ResponseBig & resp);

    /**
     * Consume the response if it exists
     */
    bool consumeResponseStream (uint64_t id, ResponseStream & resp);

    /**
     * Called when a tcpstream message is received
     * @params:
     *    - kind: the kind of message (new, old)
     *    - session: the stream to the tcp session
     */
    void onSession (net::TcpSessionKind kind, std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor existance request
     */
    void onActorExistReq (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor message
     */
    void onActorMsg (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor request (msg with response)
     */
    void onActorReq (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor request (msg with response of type cache array)
     */
    void onActorReqBig (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor request msg to open a stream between two actors
     */
    void onActorReqStream (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor response
     */
    void onActorResp (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor big response
     */
    void onActorRespBig (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat an actor big response
     */
    void onActorRespStream (std::shared_ptr <net::TcpSession> session);

    /**
     * Treat a request to kill the system
     */
    void onSystemKill ();

    /**
     * @returns: the name of the actor
     */
    std::string readActorName (std::shared_ptr <net::TcpSession> stream);

    /**
     * @returns: read an address from remote stream
     */
    net::SockAddrV4 readAddress (std::shared_ptr <net::TcpSession> stream);

    /**
     * @returns: a message
     */
    std::shared_ptr<utils::config::ConfigNode> readMessage (std::shared_ptr <net::TcpSession> stream);

    /**
     * @returns: true if the addr is the address of the local system
     */
    bool isLocal (net::SockAddrV4 addr) const;

    /**
     * @returns: an actor in the system
     */
    bool getActor (const std::string & name, std::shared_ptr <ActorBase>& act);

    /**
     * Remove an actor from the system
     */
    void removeActor (const std::string & name);

  };

}
