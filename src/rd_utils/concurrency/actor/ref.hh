#pragma once

#include <string>
#include <memory>
#include <rd_utils/net/addr.hh>
#include <rd_utils/net/stream.hh>
#include <rd_utils/utils/_.hh>
#include <rd_utils/utils/raw_parser.hh>
#include <rd_utils/memory/cache/_.hh>
#include "stream.hh"
#include <rd_utils/net/session.hh>


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

    // The connection to the actor system
    net::TcpPool _conn;

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

    /**
     * Future returned by the ref when a request is sent
     */
    class RequestStreamFuture {
      ActorSystem * _sys;
      concurrency::timer _t;
      uint64_t _reqId;
      float _timeout;
      std::shared_ptr <net::TcpSession> _session;
      std::shared_ptr <semaphore> _wait;
      RequestStreamFuture (uint64_t reqId, ActorSystem * sys, concurrency::timer t, std::shared_ptr <net::TcpSession> session, std::shared_ptr <semaphore> wait, float timeout = 5);

      friend ActorRef;

    public :

      RequestStreamFuture (const RequestStreamFuture &) = delete;
      void operator=(const RequestStreamFuture &) = delete;

      /**
       * Wait for the request to complete
       */
      std::shared_ptr <ActorStream> wait ();
    };



    /**
     * Future returned by the ref when a request is sent
     */
    template <typename T>
    class RequestBigFuture {
      ActorSystem * _sys;
      concurrency::timer _t;
      uint64_t _reqId;
      float _timeout;
      std::shared_ptr <semaphore> _wait;
      RequestBigFuture (uint64_t reqId, ActorSystem * sys, concurrency::timer t, std::shared_ptr <semaphore> wait, float timeout):
        _sys (sys)
        , _t (t)
        , _reqId (reqId)
        , _timeout (timeout)
        , _wait (wait)
      {}

      friend ActorRef;

    public :

      /**
       * Wait for the request to complete
       */
      std::shared_ptr <rd_utils::memory::cache::collection::CacheArray<T> > wait () {
        float rest = this-> _timeout;
        if (this-> _timeout > 0) {
          rest = this-> _timeout - this-> _t.time_since_start ();
          if (this-> _t.time_since_start () > this-> _timeout) {
            this-> _sys-> removeRequestId (this-> _reqId);
            throw std::runtime_error ("timeout");
          }
        }

        if (this-> _wait-> wait (rest)) {
          ActorSystem::ResponseBig resp;
          if (this-> _sys-> consumeResponseBig (this-> _reqId, resp)) {
            if (resp.msg == nullptr) throw std::runtime_error ("No response");
            return std::static_pointer_cast <rd_utils::memory::cache::collection::CacheArray <T> > (resp.msg);
          } else {
            this-> _sys-> removeRequestId (this-> _reqId);
            throw std::runtime_error ("not found");
          }
        }

        this-> _sys-> removeRequestId (this-> _reqId);
        throw std::runtime_error ("timeout");
      }
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
    void send (const rd_utils::utils::config::ConfigNode & msg, float timeout = 5);

    /**
     * Send a request to the actor
     * @params:
     *    - timeout: the timeout of the request (-1 = forever)
     */
    RequestFuture request (const rd_utils::utils::config::ConfigNode & msg, float timeout = 5);

    /**
     * Send a request to open a stream to the actor
     * @params:
     *    - timeout: the timeout of the request (-1 = forever)
     */
    RequestStreamFuture requestStream (const rd_utils::utils::config::ConfigNode & msg, float timeout = 5);

    /**
     * Send a request and wait for a big response
     * @params:
     *    - timeout: the timeout of the request (-1 = forever)
     */
    template <typename T>
    RequestBigFuture<T> requestBig (const rd_utils::utils::config::ConfigNode & msg, float timeout = 5) {
      concurrency::timer t;
      auto session = this-> _conn.get (timeout);

      session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ_BIG));
      session-> sendU32 (this-> _name.length ());
      session-> sendStr (this-> _name);
      session-> sendU32 (this-> _sys-> port ());

      auto wait = std::make_shared <semaphore> ();
      uint64_t uniqId = this-> _sys-> genUniqId ();
      this-> _sys-> registerRequestId (uniqId, wait);

      session-> sendU64 (uniqId);

      utils::raw::dump (*session, msg);
      return RequestBigFuture<T> (uniqId, this-> _sys, t, wait, timeout);
    }

    /**
     * @returns: the name of the actor
     */
    const std::string & getName () const;

    /**
     * @returns: the opened socket
     */
    net::TcpPool& getSession ();

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

    void response (uint64_t reqId, std::shared_ptr <rd_utils::utils::config::ConfigNode> msg, float timeout);

    void responseBig (uint64_t reqId, std::shared_ptr <rd_utils::memory::cache::collection::ArrayListBase> & array, float timeout);

  };

}
