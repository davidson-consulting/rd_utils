#pragma once

#include <string>
#include <memory>
#include <rd_utils/net/addr.hh>
#include <rd_utils/net/stream.hh>
#include <rd_utils/utils/_.hh>


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
