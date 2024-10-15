#pragma once

#include <string>
#include "stream.hh"

namespace rd_utils::concurrency::actor {

  class ActorSystem;
  class ActorRef;

  /**
   * Ancestor of all actor classes
   */
  class ActorBase {
  protected:

    // The name of the actor
    std::string _name;

    // The system in which the actor is running
    ActorSystem * _system;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system in which the actor is running
     * @warning: nothing actor related should be done here, the actor is not yet registered
     */
    ActorBase (const std::string & name, ActorSystem * sys);

    /**
     * Called the actor is registered in the system
     */
    virtual void onStart ();

    /**
     * Function executed when receiving a message
     */
    virtual void onMessage (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Function executed when receiving a request
     */
    virtual std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Request that returns a list of flat values
     */
    virtual std::shared_ptr <rd_utils::memory::cache::collection::ArrayListBase> onRequestList (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * When a direct stream is opened between two actors
     */
    virtual void onStream (const rd_utils::utils::config::ConfigNode & msg, ActorStream & stream);

    /**
     * @returns: the actor reference of this actor
     */
    std::shared_ptr <ActorRef> getRef ();

    /**
     * Function called to kill the actor
     */
    void exit ();

    /**
     * Called when the actor is stopped
     */
    virtual void onQuit ();

  };


}
