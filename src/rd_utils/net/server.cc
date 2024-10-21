
#ifndef __PROJECT__
#define __PROJECT__ "TCPSERVER"
#endif

#include "server.hh"

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <rd_utils/utils/_.hh>
#include <thread>
#include "session.hh"

namespace rd_utils::net {

  uint32_t getNbThreads (int32_t nb) {
    if (nb == -1) {
      auto nb = (std::thread::hardware_concurrency() / 2) ;
      if (nb == 0) return 1;
      else return nb;
    } else {
      return nb;
    }
  }

  TcpServer::TcpServer () :
     _epoll_fd (0)
    , _context (SockAddrV4 (0, 0))
    , _th (0, nullptr)
     , _trigger (false)
  {}

  TcpServer::TcpServer (SockAddrV4 v4, int nbThreads, int maxCon) :
    _epoll_fd (0)
    , _context (v4)
    ,_maxConn (maxCon)
    ,_nbThreads (getNbThreads (nbThreads))
    ,_th (0, nullptr)
    ,_trigger (true)
  {}

  TcpServer::TcpServer (TcpServer && other) :
    _epoll_fd (other._epoll_fd)
    ,_context (std::move (other._context))
    ,_maxConn (other._maxConn)
    ,_openSockets (std::move (other._openSockets))
    ,_socketFds (std::move (other._socketFds))
    ,_started (other._started)
    ,_nbThreads (other._nbThreads)
    ,_th (other._th)
    ,_runningThreads (std::move (other._runningThreads))
    ,_onSession (std::move (other._onSession))
    ,_nbSubmitted (other._nbSubmitted)
    ,_jobs (std::move (other._jobs))
    ,_nbCompleted (other._nbCompleted)
    ,_completed (std::move (other._completed))
    ,_closed (std::move (other._closed))
    ,_trigger (std::move (other._trigger))
    ,_triggerM (std::move (other._triggerM))
    ,_ready (std::move (other._ready))
    ,_waitTask (std::move (other._waitTask))
    {
      other._nbCompleted = 0;
      other._nbSubmitted = 0;
      other._th = concurrency::Thread (0, nullptr);
      other._nbThreads = 0;
      other._started = false;
      other._epoll_fd = 0;
      other._maxConn = 0;
    }

  void TcpServer::operator=(TcpServer && other) {
    this-> dispose ();

    this-> _epoll_fd = other._epoll_fd;
    this-> _context = std::move (other._context);
    this-> _maxConn = other._maxConn;
    this-> _openSockets = std::move (other._openSockets);
    this-> _socketFds = std::move (other._socketFds);
    this-> _started = other._started;
    this-> _nbThreads = other._nbThreads;
    this-> _th = other._th;
    this-> _runningThreads = std::move (other._runningThreads);
    this-> _onSession = std::move (other._onSession);
    this-> _nbSubmitted = other._nbSubmitted;
    this-> _jobs = std::move (other._jobs);
    this-> _nbCompleted = other._nbCompleted;
    this-> _completed = std::move (other._completed);
    this-> _closed = std::move (other._closed);
    this-> _trigger = std::move (other._trigger);
    this-> _triggerM = std::move (other._triggerM);
    this-> _ready = std::move (other._ready);
    this-> _waitTask = std::move (other._waitTask);

    other._nbCompleted = 0;
    other._nbSubmitted = 0;
    other._th = concurrency::Thread (0, nullptr);
    other._nbThreads = 0;
    other._started = false;
    other._epoll_fd = 0;
    other._maxConn = 0;
  }

  /**
   * =============================================================
   * =============================================================
   * ======================       MASTER      ====================
   * =============================================================
   * =============================================================
   */


  void TcpServer::start (void (*onSession)(TcpSessionKind, TcpSession&)) {
    if (this-> _started) throw utils::Rd_UtilsError ("Already running");
    this-> _onSession.dispose ();
    this-> _onSessionPtr.dispose ();
    this-> _onSession.connect (onSession);

    // need to to that in the main thread to catch the exception on binding failure
    this-> configureEpoll ();

    // Then spawning the thread with working tcplistener already configured
    this-> _th = concurrency::spawn (this, &TcpServer::pollMain);
    this-> _ready.wait ();
  }

  void TcpServer::start (void (*onSession)(TcpSessionKind, std::shared_ptr <TcpSession>)) {
    if (this-> _started) throw utils::Rd_UtilsError ("Already running");
    this-> _onSession.dispose ();
    this-> _onSessionPtr.dispose ();
    this-> _onSessionPtr.connect (onSession);

    // need to to that in the main thread to catch the exception on binding failure
    this-> configureEpoll ();

    // Then spawning the thread with working tcplistener already configured
    this-> _th = concurrency::spawn (this, &TcpServer::pollMain);
    this-> _ready.wait ();
  }


  void TcpServer::configureEpoll () {
    this-> _epoll_fd = epoll_create1 (0);

    this-> _context.start ();
    this-> _trigger.setNonBlocking ();

    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _trigger.ipipe ().getHandle ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event);

    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _context._sockfd;
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, this-> _context._sockfd, &event);
  }

  void TcpServer::addEpoll (TcpSessionKind kind, int fd) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP | EPOLLONESHOT;
    event.data.fd = fd;
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, fd, &event);
  }

  void TcpServer::delEpoll (int fd) {
    epoll_event event;
    event.data.fd = fd;
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_DEL, fd, &event);
  }

  /**
   * =============================================================
   * =============================================================
   * ====================       MAIN LOOP      ===================
   * =============================================================
   * =============================================================
   */


  void TcpServer::pollMain (concurrency::Thread) {
    this-> _started = true;
    this-> _ready.post ();

    epoll_event event;
    while (this-> _started) {
      int event_count = epoll_wait (this-> _epoll_fd, &event, 1, -1);
      if (event_count == 0) {
        throw utils::Rd_UtilsError ("Error while tcp waiting");
      }

      // std::cout << "Message from " << event.data.fd << " " << std::endl;
      this-> reloadAllFinished ();

      // New socket
      if (event.data.fd == this-> _context._sockfd) {
        try {
          TcpStream cl = this-> _context.accept ();

          // Reject connection if there are too much clients
          if (this-> _openSockets.size () + 1 > (uint32_t) this-> _maxConn && this-> _maxConn != -1) {
            cl.close ();
          } else {
            auto stream = std::make_shared <TcpStream> (std::move (cl));
            this-> _openSockets.emplace (stream-> getHandle (), stream);
            this-> _socketFds.emplace (stream, stream-> getHandle ());

            this-> submit (TcpSessionKind::NEW, stream);
          }
        } catch (utils::Rd_UtilsError & err) {
          // std::cout << this-> _openSockets.size () << std::endl;
          // std::cout << "IN ERROR ??? " << strerror (errno) << std::endl;
        } // accept can fail
      }

      // on session close
      else if (event.data.fd == this-> _trigger.ipipe ().getHandle ()) {
        char c;
        std::ignore = ::read (this-> _trigger.ipipe ().getHandle (), &c, 1);
      }

      // Old client is writing
      else {
        this-> delEpoll (event.data.fd);
        auto it = this-> _openSockets.find (event.data.fd);
        if (it != this-> _openSockets.end ()) {
          char c;
          // the socket has necessarily something to read since it triggered the epoll
          // If it does not, then it is closed and we don't want to start a session on it
          if (::recv (event.data.fd, &c, 1, MSG_PEEK) <= 0) {
            auto sock = it-> second;
            this-> _openSockets.erase (event.data.fd);
            this-> _socketFds.erase (sock);

            sock-> close ();
          } else {
            this-> submit (TcpSessionKind::OLD, it-> second);
          }
        }
      }
    }
  }

  /**
   * ===========================================================
   * ===========================================================
   * ===================       SESSION      ====================
   * ===========================================================
   * ===========================================================
   */

  void TcpServer::submit (TcpSessionKind kind, std::shared_ptr <TcpStream> stream) {
    this-> _jobs.send ({.kind = kind, .stream = stream});
    if (this-> _runningThreads.size () != (uint32_t) this-> _nbThreads) {
      this-> spawnThreads ();
    }

    this-> _nbSubmitted += 1;
    this-> _waitTask.post ();
  }

  void TcpServer::reloadAllFinished () {
    for (;;) {
      std::shared_ptr <net::TcpStream> str;
      if (this-> _completed.receive (str)) {
        if (str-> isOpen ()) {
          this-> addEpoll (TcpSessionKind::NEW, str-> getHandle ());
        } else {
          auto handle = this-> _socketFds.find (str);
          this-> _openSockets.erase (handle-> second);
          this-> _socketFds.erase (str);

          str-> close ();
        }
      } else {
        break;
      }
    }
  }

  /**
   * ===============================================================
   * ===============================================================
   * ======================        WORKERS       ===================
   * ===============================================================
   * ===============================================================
   */

  void TcpServer::spawnThreads () {
    for (int i = 0 ; i < this-> _nbThreads ; i++) {
      auto th = spawn (this, &TcpServer::workerThread);
      this-> _ready.wait ();

      this-> _runningThreads.emplace (th.id, th);
    }
  }

  void TcpServer::workerThread (concurrency::Thread t) {
    this-> _ready.post ();

    for (;;) {
      this-> _waitTask.wait ();
      if (!this-> _started) break;

      for (;;) {
        MailElement elem;
        if (this-> _jobs.receive (elem)) {
          auto session = std::make_shared <net::TcpSession> (elem.stream, this);
          if (this-> _onSessionPtr.isEmpty ()) {
            this-> _onSession.emit (elem.kind, *session);
          } else {
            this-> _onSessionPtr.emit (elem.kind, session);
          }
        } else {
          break;
        }
      }
    }

    this-> _closed.send (t.id);
  }

  void TcpServer::release (std::shared_ptr <net::TcpStream> stream) {
    this-> _completed.send (stream);
    WITH_LOCK (this-> _triggerM) {
      this-> _nbCompleted += 1;
      // trigger for epoll
      std::ignore = ::write (this-> _trigger.opipe ().getHandle (), "c", 1);
    }
  }

  /**
   * ===============================================================
   * ===============================================================
   * ======================        GETTERS       ===================
   * ===============================================================
   * ===============================================================
   */


  int TcpServer::port () const {
    return this-> _context.port ();
  }

  int TcpServer::nbConnected () const {
    return this-> _openSockets.size ();
  }

  /**
   * ===============================================================
   * ===============================================================
   * ======================        CLOSING       ===================
   * ===============================================================
   * ===============================================================
   */

  bool TcpServer::join () {
    if (!this-> isWorkerThread ()) {
      concurrency::join (this-> _th);
      return true;
    }

    return false;
  }

  void TcpServer::stop () {
    if (this-> _started) {
      WITH_LOCK (this-> _triggerM) {
        this-> _started = false;
        std::ignore = ::write (this-> _trigger.opipe ().getHandle (), "c", 1);
      }
    }
  }

  void TcpServer::waitAllCompletes () {
    // A worker thread cannot wait for completion, it will lock by waiting itself
    if (this-> isWorkerThread ()) return;

    if (this-> _started) { // Stop the submissions of new tasks
      this-> _started = false;
      // Notify the polling thread to stop
      std::ignore = ::write (this-> _trigger.opipe ().getHandle (), "c", 1);
      concurrency::join (this-> _th); // waiting for the main thread epolling
    }

    // We could try closing the open socket, but I don't really know what will happen if they are reading/writing
    //
    while (this-> _nbSubmitted != this-> _nbCompleted) { // wait for the finish of already running tasks
      char c;
      int x = ::read (this-> _trigger.ipipe ().getHandle (), &c, 1);
      if (x != 1) break;
    }
  }

  bool TcpServer::isWorkerThread () const {
    pthread_t self = concurrency::Thread::self ();
    if (this-> _th.equals (self)) return true; // main thread is a worker thread, it cannot wait itself
    for (auto &it : this-> _runningThreads) { // task pool threads, that also cannot wait themselves
      if (it.second.equals (self)) return true;
    }

    return false;
  }


  void TcpServer::dispose () {
    this-> waitAllCompletes ();
    while (this-> _closed.len () != this-> _runningThreads.size ()) {
      // Force all thread to halt
      this-> _waitTask.post ();
    }

    for (auto [it, th] : this-> _runningThreads) { // kill all running threads
      concurrency::join (th);
    }

    if (this-> _epoll_fd != 0) { // clear epoll list
      ::close (this-> _epoll_fd);
      this-> _epoll_fd = 0;
    }

    this-> _openSockets.clear ();
    this-> _context.close ();
    this-> _trigger.dispose ();
    this-> _runningThreads.clear ();
    this-> _closed.clear ();
    this-> _nbCompleted = 0;
    this-> _nbSubmitted = 0;
    this-> _started = false;
  }

  TcpServer::~TcpServer () {
    this-> dispose ();
  }

}
