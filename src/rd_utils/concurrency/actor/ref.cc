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

  ActorRef::ActorRef (bool local, const std::string & name, net::SockAddrV4 addr, std::shared_ptr<net::TcpPool> & conn, ActorSystem * sys) :
    _isLocal (local)
    , _name (name)
    , _addr (addr)
    , _conn (conn)
    , _sys (sys)
  {}

  ActorRef::RequestFuture::RequestFuture (uint64_t reqId, ActorSystem * sys, concurrency::timer t, std::shared_ptr <semaphore> wait, float timeout) :
    _sys (sys)
    , _t (t)
    , _reqId (reqId)
    , _timeout (timeout)
    , _wait (wait)
  {}


  ActorRef::RequestStreamFuture::RequestStreamFuture (uint64_t reqId, ActorSystem * sys, concurrency::timer t, net::TcpSession && s, std::shared_ptr <semaphore> wait, float timeout) :
    _sys (sys)
    , _t (t)
    , _reqId (reqId)
    , _timeout (timeout)
    , _session (std::move (s))
    , _wait (wait)
  {}

  ActorRef::RequestStreamFuture::RequestStreamFuture (ActorRef::RequestStreamFuture && other) :
    _sys (other._sys)
    , _t (other._t)
    , _reqId (other._reqId)
    , _timeout (other._timeout)
    , _session (std::move (other._session))
    , _wait (std::move (other._wait))
  {}


  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ======================================          SEND          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorRef::send (const rd_utils::utils::config::ConfigNode & node, float timeout) {
    auto session = this-> _conn-> get (timeout);

    auto & s = *session;
    s.sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_MSG));
    s.sendU32 (this-> _name.length ());
    s.sendStr (this-> _name);
    s.sendU32 (this-> _sys-> port ());

    utils::raw::dump (s, node);
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
    auto session = this-> _conn-> get (timeout);

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
    throw std::runtime_error ("timeout");

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
    auto session = this-> _conn-> get (timeout);

    session-> sendU32 ((uint32_t) ActorSystem::Protocol::ACTOR_REQ_STREAM);
    session-> sendU32 (this-> _name.length ());
    session-> sendStr (this-> _name);
    session-> sendU32 (this-> _sys-> port ());

    auto wait = std::make_shared <semaphore> ();
    uint64_t uniqId = this-> _sys-> genUniqId ();
    this-> _sys-> registerRequestId (uniqId, wait);

    session-> sendU64 (uniqId);
    utils::raw::dump (*session, msg);

    return RequestStreamFuture (uniqId, this-> _sys, t, std::move (session), wait, timeout);
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
        return std::make_shared <ActorStream> (std::move (*resp.stream), std::move (this-> _session), true);
      } else {
        this-> _sys-> removeRequestId (this-> _reqId);
        throw std::runtime_error ("not found");
      }
    }

    this-> _sys-> removeRequestId (this-> _reqId);
    throw std::runtime_error ("timeout");
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          RESPONSES          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorRef::response (uint64_t reqId, std::shared_ptr <rd_utils::utils::config::ConfigNode> node, float timeout) {
    auto session = this-> _conn-> get (timeout);
    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP));
    session-> sendU64 (reqId);

    if (node == nullptr) {
      session-> sendU32 (0);
    } else {
      session-> sendU32 (1);
      utils::raw::dump (*session, *node);
    }
  }

  void ActorRef::responseBig (uint64_t reqId, std::shared_ptr <rd_utils::memory::cache::collection::ArrayListBase> & array, float timeout) {
    auto session = this-> _conn-> get (timeout);

    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP_BIG));
    session-> sendU64 (reqId);

    if (array == nullptr) {
      session-> sendU32 (0);
    } else {
      session-> sendU32 (1);
      array-> send (*session, ARRAY_BUFFER_SIZE);
    }
  }

  std::shared_ptr <net::TcpPool> ActorRef::getSession () {
    return this-> _conn;
  }

  const std::string & ActorRef::getName () const {
    return this-> _name;
  }

  void ActorRef::dispose () {
    this-> _conn = nullptr;
  }

  ActorRef::~ActorRef () {
    this-> dispose ();
  }

}
