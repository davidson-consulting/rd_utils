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

    // The connection to the actor system
    std::shared_ptr <net::TcpStream> _conn;

    // The local actor system used to serialize messages
    ActorSystem * _sys;

  public:


    /**
     * @params:
     *    - name: the name of the actor
     *    - sock: the socket of the actor
     */
    ActorRef (bool local, const std::string & name, net::SockAddrV4 addr, std::shared_ptr <net::TcpStream> & conn, ActorSystem * sys);

    /**
     * Send a message to the actor
     */
    void send (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Send a request to the actor
     */
    std::shared_ptr<rd_utils::utils::config::ConfigNode> request (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Send a request and wait for a big response
     */
    template <typename T>
    std::shared_ptr <rd_utils::memory::cache::collection::CacheArray<T> > requestBig (const rd_utils::utils::config::ConfigNode & msg) {
      if (this-> _conn == nullptr || !this-> _conn-> isOpen ()) {
        try {
          this-> _conn = std::make_shared<net::TcpStream> (this-> _addr);
          this-> _conn-> connect ();
        } catch (...) {
          throw std::runtime_error ("Failed to reconnect to remote actor system : " + this-> _addr.toString ());
        }
      }

      this-> _conn-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ_BIG));
      this-> _conn-> sendInt (this-> _name.length ());
      this-> _conn-> send (this-> _name.c_str (), this-> _name.length ());
      this-> _conn-> sendInt (this-> _sys-> port ());

      utils::raw::dump (*this-> _conn, msg);

      uint32_t size = this-> _conn-> receiveInt ();
      std::cout << size << std::endl;

      auto result = std::make_shared <rd_utils::memory::cache::collection::CacheArray<T> > (size);

      result-> recv (*this-> _conn, ARRAY_BUFFER_SIZE);
      return result;
    }

    /**
     * @returns: the name of the actor
     */
    const std::string & getName () const;

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

    void response (uint64_t reqId, const rd_utils::utils::config::ConfigNode & msg);

  };

}
