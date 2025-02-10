#define LOG_LEVEL 10
#ifndef __PROJECT__
#define __PROJECT__ "ACTOR_REF"
#endif

#include "sys.hh"
#include <rd_utils/utils/_.hh>
#include <rd_utils/utils/raw_parser.hh>
#include "ref.hh"


namespace rd_utils::concurrency::actor {

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          CTORS          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  ActorRef::ActorRef (bool local, const std::string & name, net::SockAddrV4 addr, ActorSystem * sys) :
    _isLocal (local)
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

  void ActorRef::send (const rd_utils::utils::config::ConfigNode & node) {
    for (uint64_t i = 0 ; i < 16 ; i++) {
      try {
        auto session = this-> stream ();

        session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_MSG));
        session-> sendU32 (this-> _name.length ());
        session-> sendStr (this-> _name);
        session-> sendU32 (this-> _sys-> port ());

        utils::raw::dump (*session, node);
        if (session-> receiveU32 () == 1) { return; }
      } catch (...) {}
      LOG_ERROR ("Failed to send message");
    }

    throw std::runtime_error ("Failed to send message after multiple tries");
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          SIMPLE REQUEST          =================================
   * ====================================================================================================
   * ====================================================================================================
   */


  ActorRef::RequestFuture ActorRef::request (const rd_utils::utils::config::ConfigNode & node, float timeout) {
    bool success = false;

    concurrency::timer t;
    uint64_t uniqId = this-> _sys-> genUniqId ();
    std::shared_ptr <semaphore> wait = std::make_shared <semaphore> ();
    this-> _sys-> registerRequestId (uniqId, wait);

    for (uint64_t i = 0 ; i < 16 ; i++) {
      try {
        auto session = this-> stream ();

        session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ));
        session-> sendU32 (this-> _name.length ());
        session-> sendStr (this-> _name);
        session-> sendU32 (this-> _sys-> port ());
        session-> sendU64 (uniqId);

        utils::raw::dump (*session, node);
        if (session-> receiveU32 () == 1) {
          success = true;
          break;
        }
      } catch (...) {}

      LOG_ERROR ("Failed to send request");
    }

    if (success) {
      return RequestFuture (uniqId, this-> _sys, t, wait, timeout);
    } else {
      this-> _sys-> removeRequestId (uniqId);
    }

    throw std::runtime_error ("Failed to send request");
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
    for (uint64_t i = 0 ; i < 16 ; i++) {
      try {
        auto session = this-> stream ();
        session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP));
        session-> sendU64 (reqId);

        if (node == nullptr) {
          session-> sendU32 (0);
        } else {
          session-> sendU32 (1);
          utils::raw::dump (*session, *node);
        }

        if (session-> receiveU32 () == 1) {
          return;
        }
      } catch (...) {}

      LOG_ERROR ("Failed to send response");
    }

    throw std::runtime_error ("Failed to send response after multiple retries");
  }

  std::shared_ptr <net::TcpStream> ActorRef::stream () {
    auto s = std::make_shared <net::TcpStream> (this-> _addr);

    s-> setSendTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));
    s-> setRecvTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));

    try {
      s-> connect ();
    } catch (utils::Rd_UtilsError & err) { // failed to connect
      throw std::runtime_error ("Failed to connect : " + this-> _addr.toString () + " (" + err.what () + ")");
    }    

    return s;
  }

  const std::string & ActorRef::getName () const {
    return this-> _name;
  }

  void ActorRef::dispose () {}

  ActorRef::~ActorRef () {
    this-> dispose ();
  }

}
