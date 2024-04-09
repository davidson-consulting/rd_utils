#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <rd_utils/net/stream.hh>
#include <rd_utils/net/server.hh>
#include <rd_utils/concurrency/mutex.hh>
#include <rd_utils/concurrency/mailbox.hh>
#include <rd_utils/utils/_.hh>

namespace rd_utils::concurrency::actor {

  class ActorRef;
  class ActorBase;

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


    // The server used to communicate between actors
    net::TcpServer _server;

    // The connection to the local server
    std::shared_ptr <net::TcpStream> _localConn;

    // The list of local actors
    std::map <std::string, std::shared_ptr <ActorBase> > _actors;

    // Mutexes of actors used to make sure actors only manage one message at a time
    std::map <std::string, concurrency::mutex> _actorMutexes;

    // Connection to remote actor systems
    std::map <std::string, std::shared_ptr<net::TcpStream> > _conn;

    // True while the system is running
    bool _isRunning = false;

    // mailbox of responses
    concurrency::Mailbox <Response> _responses;

    // Semaphore when a response is available
    semaphore _waitResponse;

    // Uniq id
    uint64_t _lastU;

  public:

    enum class Protocol : int {
      ACTOR_EXIST_REQ,
      ACTOR_MSG,
      ACTOR_REQ,
      ACTOR_REQ_BIG,
      ACTOR_RESP
    };

  public:

    /**
     * @params:
     *    - addr: the accepted by the actor system
     *    - nbThreads: the number of threads used (-1 means nb cores available on system)
     */
    ActorSystem (net::SockAddrV4 addr, int nbThreads = -1, int maxCon = -1);

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

      this-> _actors.emplace (name, std::make_shared <T> (name, this));
      this-> _actorMutexes.emplace (name, concurrency::mutex ());
    }

    /**
     * Remove an actor
     * @params:
     *    - name: the name of the actor to remove
     * @info: does nothing if the actor does not exist
     */
    void remove (const std::string & name);

    /**
     * @returns: an actor reference to a local actor
     */
    std::shared_ptr<ActorRef> localActor (const std::string & name);

    /**
     * @returns: an actor reference to a remote actor
     * @params:
     *    - check: ask remote if the actor exists
     */
    std::shared_ptr<ActorRef> remoteActor (const std::string & name, net::SockAddrV4 addr, bool check = true);

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

    /**
     * Push a response
     */
    void pushResponse (Response rep);

    /**
     * generate a uniq id
     */
    uint64_t genUniqId ();

    /**
     * Called when a tcpstream message is received
     * @params:
     *    - kind: the kind of message (new, old)
     *    - session: the stream to the tcp session
     */
    void onSession (net::TcpSessionKind kind, net::TcpStream & session);

    /**
     * Treat an actor existance request
     */
    void onActorExistReq (net::TcpStream & session);

    /**
     * Treat an actor message
     */
    void onActorMsg (net::TcpStream & session);

    /**
     * Treat an actor request (msg with response)
     */
    void onActorReq (net::TcpStream & session);

    /**
     * Treat an actor request (msg with response of type cache array)
     */
    void onActorReqBig (net::TcpStream & session);

    /**
     * Treat an actor response
     */
    void onActorResp (net::TcpStream & session);

    /**
     * @returns: the name of the actor
     */
    std::string readActorName (net::TcpStream & stream);

    /**
     * @returns: read an address from remote stream
     */
    net::SockAddrV4 readAddress (net::TcpStream & stream);

    /**
     * @returns: a message
     */
    std::shared_ptr<utils::config::ConfigNode> readMessage (net::TcpStream & stream);

  };


}
