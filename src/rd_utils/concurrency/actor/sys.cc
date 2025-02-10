#define LOG_LEVEL 10

#ifndef __PROJECT__
#define __PROJECT__ "ACTOR_SYSTEM"
#endif

#include "sys.hh"
#include <sys/epoll.h>
#include <thread>
#include <rd_utils/utils/raw_parser.hh>


#include "ref.hh"
#include "base.hh"

namespace rd_utils::concurrency::actor {

  ActorSystem::ActorSystem (net::SockAddrV4 addr, int nbThreads, int nbManageThreads)
    : _addr (addr)
    , _listener (nullptr)
    , _nbJobThreads (nbThreads <= 0 ? std::thread::hardware_concurrency() : nbThreads + 1)
    , _nbManageThreads (nbManageThreads <= 0 ? std::thread::hardware_concurrency() : nbManageThreads + 1)
    , _th (0, nullptr)
    , _treatTh (0, nullptr)
  {}


  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          MASTER          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::start () {
    if (this-> _isRunning) throw std::runtime_error ("Already running");

    // need to to that in the main thread to catch the exception on binding failure
    this-> configureEpoll ();

    // Then spawning the thread with working tcplistener already configured
    this-> _th = concurrency::spawn (this, &ActorSystem::pollMain);
    this-> _ready.wait ();

    this-> _treatTh = spawn (this, &ActorSystem::workerTreatThread);
    this-> _ready.wait ();
  }

  void ActorSystem::configureEpoll () {
    this-> _epoll_fd = epoll_create1 (0);
    this-> _listener = std::make_shared <net::TcpListener> (this-> _addr);    
    this-> _listener-> start ();
    this-> _trigger.setNonBlocking ();

    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _trigger.ipipe ().getHandle ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event);

    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _listener-> getHandle ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, this-> _listener-> getHandle (), &event);
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          MAIN LOOP          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::pollMain (concurrency::Thread) {
    this-> _isRunning = true;
    this-> _ready.post ();
    
    epoll_event event;
    while (this-> _isRunning) {
      int event_count = epoll_wait (this-> _epoll_fd, &event, 1, -1);
      if (event_count == 0) {
        throw std::runtime_error ("Error while tcp waiting");
      }

      if (event.data.fd == this-> _listener-> getHandle ()) { // -> New socket
        try {
          auto stream = this-> _listener-> accept ();
          this-> _treat.send (stream);
          this-> _waitTreatTask.post ();

        } catch (const std::runtime_error & err) {
          LOG_WARN ("Connection failed on accept. ignoring.");
        }
      } else if (event.data.fd == this-> _trigger.ipipe ().getHandle ()) {  // -> triggered the trigger, maybe to tell the server to stop
        char c;
        std::ignore = ::read (this-> _trigger.ipipe ().getHandle (), &c, 1);
      }
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          SESSION          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */


  void ActorSystem::submitJob (uint32_t id, std::shared_ptr <net::TcpStream> stream) {
    this-> _jobs.send ({.protId = id, .stream = stream});
    if (this-> _runningJobThreads.size () != (uint32_t) this-> _nbJobThreads) {
      this-> spawnJobThreads ();
    }

    this-> _waitJobTask.post ();
  }

  void ActorSystem::submitManage (uint32_t id, std::shared_ptr <net::TcpStream> stream) {
    this-> _manages.send ({.protId = id, .stream = stream});
    if (this-> _runningManageThreads.size () != (uint32_t) this-> _nbManageThreads) {
      this-> spawnManageThreads ();
    }

    this-> _waitManageTask.post ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          WORKER          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::spawnJobThreads () {
    uint32_t start = this-> _runningJobThreads.size ();
    for (uint32_t i = start ; i < this-> _nbJobThreads ; i++) {
      auto th = spawn (this, &ActorSystem::workerJobThread);
      this-> _ready.wait ();

      WITH_LOCK (this-> _triggerM) {
        this-> _runningJobThreads.emplace (th.id, th);
      }
    }
  }

  void ActorSystem::spawnManageThreads () {
    uint32_t start = this-> _runningManageThreads.size ();
    for (uint32_t i = start ; i < this-> _nbManageThreads ; i++) {
      auto th = spawn (this, &ActorSystem::workerManageThread);
      this-> _ready.wait ();

      WITH_LOCK (this-> _triggerM) {
        this-> _runningManageThreads.emplace (th.id, th);
      }
    }
  }

  void ActorSystem::workerTreatThread (concurrency::Thread t) {
    this-> _ready.post ();
    
    for (;;) {
      this-> _waitTreatTask.wait ();
      if (!this-> _isRunning) break;

      std::shared_ptr <net::TcpStream> stream;
      if (this-> _treat.receive (stream)) {
        stream-> setSendTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));
        stream-> setRecvTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));

        auto protId = stream-> receiveU32 ();
        switch ((ActorSystem::Protocol) protId) {
        case ActorSystem::Protocol::ACTOR_MSG:
        case ActorSystem::Protocol::ACTOR_REQ:
          this-> submitJob (protId, stream);
          break;
        default :
          this-> submitManage (protId, stream);
        }
      } else {
        break;
      }
    }
  }
  
  void ActorSystem::workerJobThread (concurrency::Thread t) {
    this-> _ready.post ();

    for (;;) {
      this-> _waitJobTask.wait ();
      if (!this-> _isRunning) break;

      Job job;
      if (this-> _jobs.receive (job)) {
        this-> onSession (job.protId, job.stream);
      } else {
        break;
      }      
    }

    WITH_LOCK (this-> _triggerM) {
      this-> _runningJobThreads.erase (t.id);
    }
  }

  void ActorSystem::workerManageThread (concurrency::Thread t) {
    this-> _ready.post ();

    for (;;) {
      this-> _waitManageTask.wait ();
      if (!this-> _isRunning) break;

      Job job;
      if (this-> _manages.receive (job)) {
        this-> onManage (job.protId, job.stream);
      } else {
        break;
      }
    }

    WITH_LOCK (this-> _triggerM) {
      this-> _runningManageThreads.erase (t.id);
    }
  }


  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          SETTERS          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  ActorSystem& ActorSystem::joinOnEmpty (bool join) {
    this-> _stopOnEmpty = join;
    return *this;
  }

  void ActorSystem::removeActor (const std::string & name) {
    WITH_LOCK (this-> _actMut) {
      this-> _actors.erase (name);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          GETTERS          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <ActorRef> ActorSystem::localActor (const std::string & name) {
    auto addr = net::SockAddrV4 (net::Ipv4Address ("127.0.0.1"), this-> port ());
    return std::make_shared<ActorRef> (true, name, addr, this);
  }

  std::shared_ptr<ActorRef> ActorSystem::remoteActor (const std::string & name, net::SockAddrV4 addr) {
    return std::make_shared<ActorRef> (false, name, addr, this);
  }

  uint32_t ActorSystem::port () {
    if (this-> _listener == nullptr) return 0;
    return this-> _listener-> port ();
  }

  bool ActorSystem::getActor (const std::string & name, std::shared_ptr <ActorBase>& act) {
    WITH_LOCK (this-> _actMut) {
      auto it = this-> _actors.find (name);
      if (it == this-> _actors.end ()) return false;

      act = it-> second;
      return true;
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ======================================          EXIT          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::remove (const std::string & name, bool lock) {
    std::shared_ptr <ActorBase> act;
    if (this-> getActor (name, act)) {
      this-> removeActor (name);

      try {
        if (lock) {
          WITH_LOCK (act-> getMutex ()) {
            act-> onQuit ();
          }
        } else {
          act-> onQuit ();
        }
      } catch (...) {
        LOG_ERROR ("Actor ", name, " crashed on exit. Continuing..");
      }
    }

    if (this-> _stopOnEmpty && this-> _actors.size () == 0) {
      this-> poisonPill ();
    }
  }

  void ActorSystem::poisonPill () {
    try {
      auto local = this-> localActor ("");
      auto session = local-> stream ();
      session-> sendU32 ((uint32_t) ActorSystem::Protocol::SYSTEM_KILL_ALL);
    } catch (...) {
      LOG_ERROR ("Failed to connect to local system... Aborting.");
      this-> onSystemKill ();
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  uint64_t ActorSystem::genUniqId () {
    WITH_LOCK (this-> _rqMut) {
      this-> _lastU += 1;
      return this-> _lastU;
    }
  }

  void ActorSystem::registerRequestId (uint64_t id, std::shared_ptr <semaphore> who) {
    WITH_LOCK (this-> _rqMut) {
      this-> _requestIds.emplace (id, who);
    }
  }

  void ActorSystem::removeRequestId (uint64_t id) {
    WITH_LOCK (this-> _rqMut) {
      this-> _requestIds.erase (id);
    }
  }

  void ActorSystem::pushResponse (Response && rep) {
    WITH_LOCK (this-> _rqMut) {
      auto it = this-> _requestIds.find (rep.reqId);
      if (it == this-> _requestIds.end ()) {
        LOG_WARN ("Request response after timeout : ", rep.reqId);
        return;
      }

      this-> _responses.emplace (rep.reqId, std::move (rep));
      it-> second-> post ();
      this-> _requestIds.erase (rep.reqId);
    }
  }

  bool ActorSystem::consumeResponse (uint64_t id, Response & resp) {
    WITH_LOCK (this-> _rqMut) {
      auto fnd = this-> _responses.find (id);
      if (fnd != this-> _responses.end ()) {
        resp = fnd-> second;
        this-> _responses.erase (id);

        return true;
      }
    }

    return false;
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          DISPOSING          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::waitAllCompletes () {
    // A worker thread cannot wait for completion, it will lock by waiting itself
    if (this-> isWorkerThread ()) return;

    if (this-> _isRunning) { // Stop the submissions of new tasks
      // Notify the polling thread to stop
      WITH_LOCK (this-> _triggerM) {
        this-> _isRunning = false;
        std::ignore = ::write (this-> _trigger.opipe ().getHandle (), "c", 1);
      }

      concurrency::join (this-> _th); // waiting for the main thread epolling

      this-> _waitTreatTask.post ();      
      concurrency::join (this-> _treatTh);
      this->  _th = concurrency::Thread (0, nullptr);
      this-> _treatTh = concurrency::Thread (0, nullptr);
    }

    this-> _jobs.clear ();
    for (;;) {
      uint32_t size = 0;
      WITH_LOCK (this-> _triggerM) {
        size = this-> _runningJobThreads.size ();
      }

      if (size != 0) {
        this-> _waitJobTask.post ();
      } else break;
    }

    this-> _manages.clear ();
    for (;;) {
      uint32_t size = 0;
      WITH_LOCK (this-> _triggerM) {
        size = this-> _runningManageThreads.size ();
      }

      if (size != 0) {
        this-> _waitManageTask.post ();
      } else break;
    }
  }


  bool ActorSystem::dispose () {
    if (this-> isWorkerThread ()) return false;

    WITH_LOCK (this-> _actMut) {
      for (auto & it : this-> _actors) {
        if (it.second-> isAtomic ()) {
          it.second-> getMutex ().lock ();
        }

        try {
          it.second-> onQuit ();
        } catch (...) {
          LOG_ERROR ("Actor ", it.first, " crashed on exit. Continuing..");
        }

        if (it.second-> isAtomic ()) {
          it.second-> getMutex ().unlock ();
        }
      }
    }

    this-> waitAllCompletes ();
    this-> _actors.clear ();
    this-> _listener.reset ();

    return true;
  }

  bool ActorSystem::isWorkerThread () const {
    pthread_t self = concurrency::Thread::self ();
    if (this-> _th.equals (self)) return true; // main thread is a worker thread, it cannot wait itself
    for (auto &it : this-> _runningJobThreads) { // task pool threads, that also cannot wait themselves
      if (it.second.equals (self)) return true;
    }

    for (auto &it : this-> _runningManageThreads) { // task pool threads, that also cannot wait themselves
      if (it.second.equals (self)) return true;
    }

    return false;
  }

  bool ActorSystem::join () {
    if (!this-> isWorkerThread ()) {
      concurrency::join (this-> _th);
      return true;
    }

    return false;
  }

  ActorSystem::~ActorSystem () {
    this-> dispose ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          SESSION          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::onManage (uint32_t protId, std::shared_ptr <net::TcpStream> session) {
    try {
      switch ((ActorSystem::Protocol) protId) {
      case ActorSystem::Protocol::ACTOR_EXIST_REQ:
        this-> onActorExistReq (session);
        break;
      case ActorSystem::Protocol::ACTOR_RESP:
        this-> onActorResp (session);
        break;
      case ActorSystem::Protocol::SYSTEM_KILL_ALL:
        this-> onSystemKill ();
        break;
      default :
        break;
      }
    } catch (...) {
      LOG_INFO ("Session failed. Remote connection out.");
    }
  }

  void ActorSystem::onSession (uint32_t protId, std::shared_ptr <net::TcpStream> session) {
    try {
      switch ((ActorSystem::Protocol) protId) {
      case ActorSystem::Protocol::ACTOR_MSG:
        this-> onActorMsg (session);
        break;
      case ActorSystem::Protocol::ACTOR_REQ:
        this-> onActorReq (session);
        break;
      default :
        break;
      }
    } catch (...) {
      LOG_INFO ("Session failed. Remote connection out.");
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::onActorExistReq (std::shared_ptr <net::TcpStream> session) {
    std::string name;
    try {
      name = this-> readActorName (session);
      LOG_DEBUG ("Actor existence : ", name);
      session-> sendU32 (1);
    } catch (...) {
      LOG_ERROR ("Failed to read request");
      session-> sendU32 (0, false);
      return;
    }

    std::shared_ptr <ActorBase> act;
    if (this-> getActor (name, act)) {
      session-> sendU32 (1, true);
    } else {
      session-> sendU32 (0, true);
    }
  }

  void ActorSystem::onActorReq (std::shared_ptr <net::TcpStream> session) {
    std::string name;
    net::SockAddrV4 addr (net::Ipv4Address (0), 0);
    uint64_t reqId;
    std::shared_ptr <utils::config::ConfigNode> msg;

    try {
      name = this-> readActorName (session);
      addr = this-> readAddress (session);
      reqId = session-> receiveU64 ();
      msg = this-> readMessage (session);
      session-> sendU32 (1);
    } catch (...) {
      LOG_ERROR ("Failed to read request");
      session-> sendU32 (0, false);
      return;
    }

    std::shared_ptr <ActorBase> act;
    if (!this-> getActor (name, act)) {
      return;
    }

    if (act-> isAtomic ()) {
      act-> getMutex ().lock ();
    }

    try {
      auto result = act-> onRequest (*msg);
      try {
        auto actorRef = this-> remoteActor ("", addr);
        actorRef-> response (reqId, result);
      } catch (...) {
        LOG_ERROR ("Failed to send response");
      }
    } catch (...) {
      LOG_ERROR ("Sesssion failed");
    }

    if (act-> isAtomic ()) {
      act-> getMutex ().unlock ();
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          MESSAGES          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::onActorMsg (std::shared_ptr <net::TcpStream> session) {
    std::string name;
    std::shared_ptr <utils::config::ConfigNode> msg;

    try {
      name = this-> readActorName (session);
      this-> readAddress (session);
      msg = this-> readMessage (session);
      session-> sendU32 (1);
    } catch (...) {
      LOG_ERROR ("Failed to read message");
      session-> sendU32 (0, false);
      return;
    }

    std::shared_ptr <ActorBase> act;
    if (!this-> getActor (name, act)) {
      return;
    }

    if (act-> isAtomic ()) {
      act-> getMutex ().lock ();
    }

    try {
      act-> onMessage (*msg);
    } catch (...) {
    }

    if (act-> isAtomic ()) {
      act-> getMutex ().unlock ();
    }
  }

  void ActorSystem::onSystemKill () {
    WITH_LOCK (this-> _actMut) {
      for (auto & it : this-> _actors) {
        WITH_LOCK (it.second-> getMutex ()) {
          try {
            it.second-> onQuit ();
          } catch (...) {
            LOG_ERROR ("Actor ", it.first, " crashed on exit. Continuing..");
          }
        }
      }
    }

    this-> _actors.clear ();

    if (this-> _isRunning) {
      WITH_LOCK (this-> _triggerM) {
        this-> _isRunning = false;
        std::ignore = ::write (this-> _trigger.opipe ().getHandle (), "c", 1);
      }
    }

    // we don't dispose the server we might be in a thread, and therefore server is waiting for this function to quit
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          RESPONSES          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::onActorResp (std::shared_ptr <net::TcpStream> session) {
    auto reqId = session-> receiveU64 ();
    auto hasValue = session-> receiveU32 ();

    if (hasValue == 1) {
      std::shared_ptr<utils::config::ConfigNode> msg = nullptr;
      try {
        msg = this-> readMessage (session);
        session-> sendU32 (1);
      } catch (std::runtime_error & e) {
        LOG_ERROR ("Failed to read request response message : ", reqId, " ", e.what ());
        session-> sendU32 (0, false);
        return;
      }

      this-> pushResponse ({.reqId = reqId, .msg = msg});
    } else {
      this-> pushResponse ({.reqId = reqId, .msg = nullptr});
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          PRIVATE          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::string ActorSystem::readActorName (std::shared_ptr <net::TcpStream> stream) {
    auto len = stream-> receiveU32 ();
    if (len <= 32) {
      return stream-> receiveStr (len);
    } throw std::runtime_error ("Name too long");
  }

  net::SockAddrV4 ActorSystem::readAddress (std::shared_ptr <net::TcpStream> stream) {
    auto port = stream-> receiveU32 ();
    auto ip = stream-> addr ().ip ();

    return net::SockAddrV4 (ip, port);
  }

  std::shared_ptr<utils::config::ConfigNode> ActorSystem::readMessage (std::shared_ptr <net::TcpStream> stream) {
    return utils::raw::parse (*stream);
  }

}
