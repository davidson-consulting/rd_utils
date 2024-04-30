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
     */
    void send (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Send a request to the actor
     */
    std::shared_ptr<rd_utils::utils::config::ConfigNode> request (const rd_utils::utils::config::ConfigNode & msg);


    std::shared_ptr<ActorStream> requestStream (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Send a request and wait for a big response
     */
    template <typename T>
    std::shared_ptr <rd_utils::memory::cache::collection::CacheArray<T> > requestBig (const rd_utils::utils::config::ConfigNode & msg) {
      auto session = this-> _conn-> get ();

      session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ_BIG));
      session-> sendU32 (this-> _name.length ());
      session-> send (this-> _name.c_str (), this-> _name.length ());
      session-> sendU32 (this-> _sys-> port ());

      uint64_t uniqId = this-> _sys-> genUniqId ();
      session-> sendU64 (uniqId);

      utils::raw::dump (*session, msg);
      for (;;) {
        this-> _sys-> _waitResponseBig.wait ();
        ActorSystem::ResponseBig resp;
        if (this-> _sys-> _responseBigs.receive (resp)) {
          if (resp.reqId == uniqId) {
            return std::static_pointer_cast <rd_utils::memory::cache::collection::CacheArray <T> > (resp.msg);
          }
        }

        this-> _sys-> pushResponseBig (resp);
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

    void response (uint64_t reqId, std::shared_ptr <rd_utils::utils::config::ConfigNode> msg);

    void responseBig (uint64_t reqId, std::shared_ptr <rd_utils::memory::cache::collection::ArrayListBase> & array);

  };

}
