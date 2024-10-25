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
    std::shared_ptr <net::TcpPool> _conn;

    // The local actor system used to serialize messages
    ActorSystem * _sys;

  public:


    /**
     * @params:
     *    - name: the name of the actor
     *    - sock: the socket of the actor
     */
    ActorRef (bool local, const std::string & name, net::SockAddrV4 addr, std::shared_ptr <net::TcpPool> & conn, ActorSystem * sys);

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
    std::shared_ptr<rd_utils::utils::config::ConfigNode> request (const rd_utils::utils::config::ConfigNode & msg, float timeout= 5);

    /**
     * Send a request to open a stream to the actor
     * @params:
     *    - timeout: the timeout of the request (-1 = forever)
     */
    std::shared_ptr<ActorStream> requestStream (const rd_utils::utils::config::ConfigNode & msg, float timeout= 5);

    /**
     * Send a request and wait for a big response
     * @params:
     *    - timeout: the timeout of the request (-1 = forever)
     */
    template <typename T>
    std::shared_ptr <rd_utils::memory::cache::collection::CacheArray<T> > requestBig (const rd_utils::utils::config::ConfigNode & msg, float timeout= 5) {
      concurrency::timer t;
      auto session = this-> _conn-> get (timeout);

      session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ_BIG));
      session-> sendU32 (this-> _name.length ());
      session-> sendStr (this-> _name);
      session-> sendU32 (this-> _sys-> port ());

      uint64_t uniqId = this-> _sys-> genUniqId ();
      this-> _sys-> registerRequestId (uniqId);

      session-> sendU64 (uniqId);

      utils::raw::dump (*session, msg);
      float rest = timeout;
      for (;;) {
        if (timeout > 0) {
          rest = timeout - t.time_since_start ();
          if (t.time_since_start () > timeout) {
            this-> _sys-> removeRequestId (uniqId);
            throw std::runtime_error ("timeout");
          }
        }

        if (this-> _sys-> _waitResponseBig.wait (rest)) {
          ActorSystem::ResponseBig resp;
          if (this-> _sys-> _responseBigs.receive (resp)) {
            if (resp.reqId == uniqId) {
              if (resp.msg == nullptr) throw std::runtime_error ("No response");
              return std::static_pointer_cast <rd_utils::memory::cache::collection::CacheArray <T> > (resp.msg);
            }
          }

          this-> _sys-> pushResponseBig (resp);
        } else {
          this-> _sys-> removeRequestId (uniqId);
          throw std::runtime_error ("timeout");
        }
      }
    }

    /**
     * @returns: the name of the actor
     */
    const std::string & getName () const;

    /**
     * @returns: the opened socket
     */
    std::shared_ptr <net::TcpPool> getSession ();

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
