#define LOG_LEVEL 10
#ifndef __PROJECT__
#define __PROJECT__ "ACTOR_REF"
#endif

#include "sys.hh"
#include <rd_utils/utils/_.hh>
#include <rd_utils/utils/raw_parser.hh>
#include "ref.hh"
#include "msg.hh"

namespace rd_utils::concurrency::actor {

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          CTORS          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  ActorRef::ActorRef (const std::string & name, net::SockAddrV4 addr, ActorSystem * sys) :
    _isLocal (sys-> isLocalAddr (addr))
    , _name (name)
    , _addr (addr)
    , _sys (sys)
  {

  }

  ActorRef::RequestFuture::RequestFuture (uint64_t reqId, ActorSystem * sys, concurrency::timer t, std::shared_ptr <semaphore> wait, float timeout) :
    _sys (sys)
    , _t (t)
    , _reqId (reqId)
    , _timeout (timeout)
    , _wait (wait)
  {}

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ======================================          SEND          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */


  void ActorRef::send (const rd_utils::utils::config::Dict & node) {
    this-> send (std::make_shared <utils::config::Dict> (node));
  }

  void ActorRef::send (std::shared_ptr <rd_utils::utils::config::ConfigNode> node) {
    ActorMessage msg (this-> _sys, ActorMessage::Protocol::ACTOR_MSG, this-> _name, node);
    bool success = this-> _sys-> sendMessage (this-> _addr, msg, 100, this-> _isLocal);
    if (!success) {
      throw std::runtime_error ("Failed to send message to " + this-> _name);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          SIMPLE REQUEST          =================================
   * ====================================================================================================
   * ====================================================================================================
   */

  ActorRef::RequestFuture ActorRef::request (const utils::config::Dict & node, float timeout) {
    return this-> request (std::make_shared <utils::config::Dict> (node), timeout);
  }

  ActorRef::RequestFuture ActorRef::request (std::shared_ptr <rd_utils::utils::config::ConfigNode> node, float timeout) {
    concurrency::timer t;
    uint64_t uniqId = this-> _sys-> genUniqId ();
    std::shared_ptr <semaphore> wait = std::make_shared <semaphore> ();
    this-> _sys-> registerRequestId (uniqId, wait);
    ActorMessage msg (this-> _sys, ActorMessage::Protocol::ACTOR_REQ, uniqId, this-> _name, node);

    bool success = this-> _sys-> sendMessage (this-> _addr, msg, 100, this-> _isLocal);
    if (success) {
      return RequestFuture (uniqId, this-> _sys, t, wait, timeout);
    } else {
      this-> _sys-> removeRequestId (uniqId);
    }

    throw std::runtime_error ("Failed to send request " + std::to_string (uniqId) + " to " + this-> _name);
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> ActorRef::RequestFuture::wait () {
    float rest = this-> _timeout;
    if (this-> _timeout > 0) {
      rest = this-> _timeout - this-> _t.time_since_start ();
      if (this-> _t.time_since_start () > this-> _timeout) {
        this-> _sys-> removeRequestId (this-> _reqId);
        throw std::runtime_error ("timeout");
      }
    }

    if (this-> _wait-> wait (rest)) {
      ActorSystem::Response resp;
      if (this-> _sys-> consumeResponse (this-> _reqId, resp)) {
        return resp.msg;
      } else {
        this-> _sys-> removeRequestId (this-> _reqId);
        throw std::runtime_error ("not found");
      }
    }

    this-> _sys-> removeRequestId (this-> _reqId);
    throw std::runtime_error ("timeout " + std::to_string (this-> _reqId));

  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          RESPONSES          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorRef::response (uint64_t reqId, std::shared_ptr <rd_utils::utils::config::ConfigNode> node) {
    ActorMessage msg (this-> _sys, ActorMessage::Protocol::ACTOR_RESP, reqId, "", node);
    bool success = this-> _sys-> sendMessage (this-> _addr, msg, 100, this-> _isLocal);
    if (!success) {
      throw std::runtime_error ("Failed to send response " + std::to_string (reqId));
    }
  }

  const std::string & ActorRef::getName () const {
    return this-> _name;
  }

  void ActorRef::dispose () {}

  ActorRef::~ActorRef () {
    this-> dispose ();
  }

}
