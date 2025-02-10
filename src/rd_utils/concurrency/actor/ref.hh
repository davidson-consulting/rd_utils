#pragma once

#include <string>
#include <memory>
#include <rd_utils/net/addr.hh>
#include <rd_utils/net/stream.hh>
#include <rd_utils/utils/_.hh>
#include <rd_utils/utils/raw_parser.hh>
#include <rd_utils/memory/cache/_.hh>


namespace rd_utils::concurrency::actor {

  class ActorSystem;

  /**
   * A reference to an actor
   */
  class ActorRef {
  private:

    // True if the actor is a local actor
    bool _isLocal;

    // The name of the actor
    std::string _name;

    // The remote address
    net::SockAddrV4 _addr;

    // The local actor system used to serialize messages
    ActorSystem * _sys;

  public:

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          FUTURES          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    /**
     * Future returned by the ref when a request is sent
     */
    class RequestFuture {
      ActorSystem * _sys;
      concurrency::timer _t;
      uint64_t _reqId;
      float _timeout;
      std::shared_ptr <semaphore> _wait;
      RequestFuture (uint64_t reqId, ActorSystem * sys, concurrency::timer t, std::shared_ptr <semaphore> wait, float timeout = 5);

      friend ActorRef;

    public:


      /**
       * Wait for the request to complete
       */
      std::shared_ptr <rd_utils::utils::config::ConfigNode> wait ();

    };

  public :

    ActorRef (const ActorRef &) = delete;
    void operator=(const ActorRef&) = delete;

    /**
     * @params:
     *    - name: the name of the actor
     *    - sock: the socket of the actor
     */
    ActorRef (bool local, const std::string & name, net::SockAddrV4 addr, ActorSystem * sys);

    /**
     * Send a message to the actor
     * @params:
     *    - msg: the message to send
     *    - timeout: timeout of the connection to the other actor
     * @info: if timeout < 0 wait indefinitely
     */
    void send (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Send a request to the actor
     * @params:
     *    - timeout: the timeout of the request (-1 = forever)
     */
    RequestFuture request (const rd_utils::utils::config::ConfigNode & msg, float timeout = 5);

    /**
     * @returns: the name of the actor
     */
    const std::string & getName () const;

    /**
     * @returns: an opened socket to the remote actor system
     */
    std::shared_ptr <net::TcpStream> stream ();

    /**
     * Close the actor reference
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~ActorRef ();

  private :

    friend ActorSystem;

    void response (uint64_t reqId, std::shared_ptr <rd_utils::utils::config::ConfigNode> msg);

  };

}
