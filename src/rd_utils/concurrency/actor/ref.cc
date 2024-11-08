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


  ActorRef::RequestStreamFuture::RequestStreamFuture (uint64_t reqId, ActorSystem * sys, concurrency::timer t, std::shared_ptr <net::TcpStream> s, std::shared_ptr <semaphore> wait, float timeout) :
    _sys (sys)
    , _t (t)
    , _reqId (reqId)
    , _timeout (timeout)
    , _str (std::move (s))
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
    auto session = this-> stream ();

    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_MSG));
    session-> sendU32 (this-> _name.length ());
    session-> sendStr (this-> _name);
    session-> sendU32 (this-> _sys-> port ());

    utils::raw::dump (*session, node);
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          SIMPLE REQUEST          =================================
   * ====================================================================================================
   * ====================================================================================================
   */


  ActorRef::RequestFuture ActorRef::request (const rd_utils::utils::config::ConfigNode & node, float timeout) {
    concurrency::timer t;
    auto session = this-> stream ();

    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ));
    session-> sendU32 (this-> _name.length ());
    session-> sendStr (this-> _name);
    session-> sendU32 (this-> _sys-> port ());

    std::shared_ptr <semaphore> wait = std::make_shared <semaphore> ();
    uint64_t uniqId = this-> _sys-> genUniqId ();
    this-> _sys-> registerRequestId (uniqId, wait);

    session-> sendU64 (uniqId);

    utils::raw::dump (*session, node);

    return RequestFuture (uniqId, this-> _sys, t, wait, timeout);
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
   * ===================================          STREAM REQ          ===================================
   * ====================================================================================================
   * ====================================================================================================
   */

  ActorRef::RequestStreamFuture ActorRef::requestStream (const rd_utils::utils::config::ConfigNode & msg, float timeout) {
    concurrency::timer t;
    auto session = this-> stream ();

    session-> sendU32 ((uint32_t) ActorSystem::Protocol::ACTOR_REQ_STREAM);
    session-> sendU32 (this-> _name.length ());
    session-> sendStr (this-> _name);
    session-> sendU32 (this-> _sys-> port ());

    auto wait = std::make_shared <semaphore> ();
    uint64_t uniqId = this-> _sys-> genUniqId ();
    this-> _sys-> registerRequestId (uniqId, wait);

    session-> sendU64 (uniqId);
    utils::raw::dump (*session, msg);

    return RequestStreamFuture (uniqId, this-> _sys, t, session, wait, timeout);
  }

  std::shared_ptr <ActorStream> ActorRef::RequestStreamFuture::wait () {
    float rest = this-> _timeout;
    if (this-> _timeout > 0) {
      rest = this-> _timeout - this-> _t.time_since_start ();
      if (this-> _t.time_since_start () > this-> _timeout) {
        this-> _sys-> removeRequestId (this-> _reqId);
        throw std::runtime_error ("timeout");
      }
    }

    if (this-> _wait-> wait (rest)) {
      ActorSystem::ResponseStream resp;
      if (this-> _sys-> consumeResponseStream (this-> _reqId, resp)) {
        return std::make_shared <ActorStream> (resp.stream, this-> _str);
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
    {
      auto session = this-> stream ();
      session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP));
      session-> sendU64 (reqId);

      if (node == nullptr) {
        session-> sendU32 (0);
      } else {
        session-> sendU32 (1);
        utils::raw::dump (*session, *node);
      }
    }
  }

  std::shared_ptr <net::TcpStream> ActorRef::stream () {
    auto s = std::make_shared <net::TcpStream> (this-> _addr);

    s-> setSendTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));
    s-> setRecvTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));

    try {
      s-> connect ();
    } catch (utils::Rd_UtilsError & err) { // failed to connect
      throw std::runtime_error ("Failed to connect");
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
