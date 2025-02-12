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
#include "msg.hh"

namespace rd_utils::concurrency::actor {

  ActorSystem::ActorSystem (net::SockAddrV4 addr, int nbThreads)
    : _addr (addr)
    , _listener (nullptr)
    , _nbJobThreads (nbThreads <= 0 ? std::thread::hardware_concurrency() : nbThreads + 1)
    , _th (0)
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
          this-> receiveMessage (stream);
          if (stream-> isOpen ()) { // still open
            this-> addPoll (stream);
          }

        } catch (const std::runtime_error & err) {
          LOG_WARN ("Connection failed on accept. ignoring.");
        }
      } else if (event.data.fd == this-> _trigger.ipipe ().getHandle ()) {  // -> triggered the trigger, maybe to tell the server to stop
        char c;
        std::ignore = ::read (this-> _trigger.ipipe ().getHandle (), &c, 1);
      } else {
        auto sc = this-> _sockets.find (event.data.fd);
        if (sc == this-> _sockets.end ()) {
          this-> delPoll (event.data.fd);
        } else {
          this-> receiveMessage (sc-> second);
          if (!sc-> second-> isOpen ()) {
            this-> delPoll (event.data.fd);
          }
        }
      }
    }
  }

  void ActorSystem::addPoll (std::shared_ptr <net::TcpStream> socket) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = socket-> getHandle ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event);

    this-> _sockets.emplace (socket-> getHandle (), socket);
  }

  void ActorSystem::delPoll (uint32_t fd) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = fd;
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_DEL, event.data.fd, &event);

    this-> _sockets.erase (fd);
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          SESSION          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::submitJob (Job && job) {
    this-> _jobs.send (std::move (job));
    if (this-> _runningJobThreads.size () != (uint32_t) this-> _nbJobThreads) {
      this-> spawnJobThreads ();
    }

    this-> _waitJobTask.post ();
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

  void ActorSystem::workerJobThread (concurrency::Thread t) {
    this-> _ready.post ();
    Job job {.data = {}, .origin = net::Ipv4Address (0)};

    for (;;) {
      this-> _waitJobTask.wait ();
      if (!this-> _isRunning) break;

      if (this-> _jobs.receive (job)) {
        try {
          ActorMessage msg = ActorMessage::deserialize (this, job.origin, job.data);
          this-> onSession (msg);
        } catch (const std::exception & err) {
          LOG_ERROR ("Failed to read message ", err.what ());
        }
      } else {
        break;
      }      
    }

    WITH_LOCK (this-> _triggerM) {
      this-> _runningJobThreads.erase (t.id);
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
    return std::make_shared <ActorRef> (name, addr, this);
  }

  std::shared_ptr<ActorRef> ActorSystem::remoteActor (const std::string & name, net::SockAddrV4 addr) {
    return std::make_shared<ActorRef> (name, addr, this);
  }

  uint32_t ActorSystem::port () const {
    if (this-> _listener == nullptr) return 0;
    return this-> _listener-> port ();
  }

  net::SockAddrV4 ActorSystem::addr () const {
    return net::SockAddrV4 (this-> _addr.ip (), this-> port ());
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

    bool kill = false;
    WITH_LOCK (this-> _triggerM) {
      if (this-> _stopOnEmpty && this-> _actors.size () == 0) {
        kill = true;
      }
    }

    if (kill) {
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

  void ActorSystem::onSession (const ActorMessage & msg) {
    try {
      switch (msg.kind ()) {
      case ActorMessage::Protocol::ACTOR_RESP:
        this-> pushResponse ({.reqId = msg.getUniqId (), .msg = msg.getContent () });
        break;
      case ActorMessage::Protocol::ACTOR_MSG:
        this-> onActorMsg (msg);
        break;
      case ActorMessage::Protocol::ACTOR_REQ:
        this-> onActorReq (msg);
        break;
      default :
        break;
      }
    } catch (...) {
      LOG_ERROR ("Session failed. Remote connection out.");
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::onActorReq (const ActorMessage & msg) {
    std::shared_ptr <ActorBase> act;
    if (msg.getContent () == nullptr || !this-> getActor (msg.getTargetActor (), act)) {
      ActorMessage response (this, ActorMessage::Protocol::ACTOR_RESP, msg.getUniqId (), "", nullptr);
      this-> sendMessage (msg.getSenderAddr (), response, 100, this-> isLocalAddr (msg.getSenderAddr ()));
      return;
    }

    if (act-> isAtomic ()) {
      act-> getMutex ().lock ();
    }

    try {
      auto result = act-> onRequest (*msg.getContent ());
      ActorMessage response (this, ActorMessage::Protocol::ACTOR_RESP, msg.getUniqId (), "", result);
      if (!this-> sendMessage (msg.getSenderAddr (), response, 100, this-> isLocalAddr (msg.getSenderAddr ()))) {
        LOG_ERROR ("Failed to send response : " + std::to_string (msg.getUniqId ()));
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

  void ActorSystem::onActorMsg (const ActorMessage & msg) {
    std::shared_ptr <ActorBase> act;
    if (msg.getContent () == nullptr || !this-> getActor (msg.getTargetActor (), act)) {
      return;
    }

    if (act-> isAtomic ()) {
      act-> getMutex ().lock ();
    }

    try {
      act-> onMessage (*msg.getContent ());
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

  bool ActorSystem::sendMessage (net::SockAddrV4 addr, const ActorMessage & msg, uint32_t nbTries, bool local) {
    auto buffer = msg.serialize ();
    if (local)  {
      this-> submitJob (Job {.data = buffer, .origin = this-> addr ().ip ()});
      return true;
    } else {
      for (uint32_t i = 0 ; i < nbTries ; i++) {
        auto str = this-> connect (addr);
        if (str != nullptr) {
          bool success = true;
          try {
            str-> sendU32 (buffer.size (), true);
            str-> sendRaw (buffer.data (), buffer.size (), true);
          } catch (...) {
            success = false;
          }

          this-> release (addr, str);
          if (success) {
            if (i != 0) LOG_ERROR ("TOOK : ", i, " iterations ");
            return true;
          }
        }

        // concurrency::timer::sleep (0.01);
      }

      return false;
    }
  }

  void ActorSystem::receiveMessage (std::shared_ptr <net::TcpStream> stream) {
    std::vector <uint8_t> buffer;
    rd_utils::concurrency::timer t;
    try {
      auto size = stream-> receiveU32 ();
      if (size > utils::MemorySize::MB (1).bytes ()) throw std::runtime_error ("message to big");
      buffer.resize (size);
      stream-> receiveRaw (buffer.data (), size, true);
    } catch (...) {
      LOG_ERROR ("Failed to recv message");
      return;
    }

    try {
      uint32_t len = buffer.size ();
      uint8_t * content = buffer.data ();
      auto result = static_cast <ActorMessage::Protocol> (utils::raw::readU8 (content, len));
      if (result == ActorMessage::Protocol::SYSTEM_KILL_ALL) {
        this-> onSystemKill ();
      } else {
        this-> submitJob (Job {.data = buffer, .origin = stream-> addr ().ip () });
      }
    } catch (...) {
      return;
    }
  }

  bool ActorSystem::isLocalAddr (net::SockAddrV4 addr) const {
    auto local = this-> addr ();
    return addr.port () == local.port () && addr.ip ().toN () == local.ip ().toN ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ================================          CONNECTION POOL          =================================
   * ====================================================================================================
   * ====================================================================================================
   */


  std::shared_ptr <net::TcpStream> ActorSystem::connect (net::SockAddrV4 addr) {
    uint32_t value [2] = {addr.ip ().toN (), addr.port ()};
    uint64_t * r = reinterpret_cast <uint64_t*> (&value);

    WITH_LOCK (this-> _rqMut) {
      auto it = this-> _remotes.find (r [0]);
      if (it != this-> _remotes.end () && it-> second.size () > 0) {
        auto back = it-> second.back ();
        it-> second.pop_back ();
        return back;
      }
    }

    try {
      auto newConn = std::make_shared <net::TcpStream> (addr);
      newConn-> connect ();
      return newConn;
    } catch (...) {
      LOG_ERROR ("Failed to connect to : ", addr);
    }

    return nullptr;
  }

  void ActorSystem::release (net::SockAddrV4 addr, std::shared_ptr <net::TcpStream> socket) {
    if (socket-> isOpen ()) {
      uint32_t value [2] = {addr.ip ().toN (), addr.port ()};
      uint64_t * r = reinterpret_cast <uint64_t*> (&value);

      WITH_LOCK (this-> _rqMut) {
        auto it = this-> _remotes.find (r [0]);
        if (it != this-> _remotes.end ()) {
          it-> second.push_front (socket);
        } else {
          std::list <std::shared_ptr <net::TcpStream> > lst;
          lst.push_front (socket);
          this-> _remotes.emplace (r [0], std::move (lst));
        }
      }
    }
  }

}
