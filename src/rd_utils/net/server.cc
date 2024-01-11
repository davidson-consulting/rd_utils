#include "server.hh"

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <rd_utils/utils/_.hh>


namespace rd_utils::net {

  TcpServer::TcpServer () :
    _context (SockAddrV4 (0, 0))
    , _trigger (false)
    , _epoll_fd (0)
    , _th (0, nullptr)
  {}

  TcpServer::TcpServer (SockAddrV4 v4, int nbThreads, int maxCon) :
    _context (v4)
    ,_trigger (true)
    ,_nbThreads (nbThreads)
    ,_th (0, nullptr)
    ,_maxConn (maxCon)
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


  void TcpServer::start (void (*onSession)(TcpSessionKind, TcpStream&)) {
    if (this-> _started) throw utils::Rd_UtilsError ("Already running");
    this-> _onSession.dispose ();
    this-> _onSession.connect (onSession);

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
    event.data.fd = this-> _trigger.getReadFd ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, this-> _trigger.getReadFd (), &event);

    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _context._sockfd;
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, this-> _context._sockfd, &event);
  }

  void TcpServer::pollMain (concurrency::Thread) {
    this-> _started = true;
    this-> _ready.post ();

    epoll_event event;
    while (this-> _started) {
      int event_count = epoll_wait (this-> _epoll_fd, &event, 1, -1);
      if (event_count == 0) {
        throw utils::Rd_UtilsError ("Error while tcp waiting");
      }

      this-> reloadAllFinished ();

      // New socket
      if (event.data.fd == this-> _context._sockfd) {
        try {
          TcpStream cl = std::move (this-> _context.accept ());
          // Reject connection if there are too much clients
          if (this-> _openSockets.size () + 1 > this-> _maxConn && this-> _maxConn != -1) {
            cl.close ();
          } else {
            auto stream = new TcpStream (std::move (cl));
            this-> _openSockets.emplace (stream-> getHandle (), stream);
            this-> _socketFds.emplace (stream, stream-> getHandle ());

            this-> submit (TcpSessionKind::NEW, stream);
          }
        } catch (utils::Rd_UtilsError err) {
          // std::cout << this-> _openSockets.size () << std::endl;
          // std::cout << "IN ERROR ??? " << std::endl;

        } // accept can fail
      }

      // on session close
      else if (event.data.fd == this-> _trigger.getReadFd ()) {
        char c;
        ::read (this-> _trigger.getReadFd (), &c, 1);
      }

      // Old client is writing
      else {
        // this-> delEpoll (event.data.fd);
        auto it = this-> _openSockets.find (event.data.fd);
        if (it != this-> _openSockets.end ()) {
          this-> submit (TcpSessionKind::OLD, it-> second);
        }
      }
    }
  }

  void TcpServer::addEpoll (TcpSessionKind kind, int fd) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP | EPOLLONESHOT;
    event.data.fd = fd;
    if (kind == TcpSessionKind::NEW) {
      epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, fd, &event);
    } else {
      epoll_ctl (this-> _epoll_fd, EPOLL_CTL_MOD, fd, &event);
    }
  }

  void TcpServer::delEpoll (int fd) {
    epoll_event event;
    event.data.fd = fd;
    auto i = epoll_ctl (this-> _epoll_fd, EPOLL_CTL_DEL, fd, &event);
  }

  void TcpServer::submit (TcpSessionKind kind, TcpStream * stream) {
    this-> _jobs.send ({.kind = kind, .stream = stream});
    if (this-> _runningThreads.size () != this-> _nbThreads) {
      this-> spawnThreads ();
    }

    this-> _nbSubmitted += 1;
    this-> _waitTask.post ();
  }

  void TcpServer::reloadAllFinished () {
    for (;;) {
      MailElement elem;
      if (this-> _completed.receive (elem)) {
        TcpStream* str = elem.stream;

        if (str-> isOpen ()) {
          this-> addEpoll (elem.kind, str-> getHandle ());
        } else {
          this-> delEpoll (str-> getHandle ());

          auto handle = this-> _socketFds.find (str);
          this-> _openSockets.erase (handle-> second);
          this-> _socketFds.erase (str);

          str-> close ();
          delete str;
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
          this-> _onSession.emit (elem.kind, *elem.stream);
          this-> _completed.send (elem);

          WITH_LOCK (this-> _triggerM) {
            this-> _nbCompleted += 1;
            // trigger for epoll
            ::write (this-> _trigger.getWriteFd (), "c", 1);
          }
        } else {
          break;
        }
      }
    }

    this-> _closed.send (t.id);
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

  void TcpServer::join () {
    concurrency::join (this-> _th);
  }

  void TcpServer::stop () {
    WITH_LOCK (this-> _triggerM) {
      this-> _started = false;
      ::write (this-> _trigger.getWriteFd (), "c", 1);
    }
  }

  void TcpServer::waitAllCompletes () {
    if (this-> _started) { // Stop the submissions of new tasks
      this-> _started = false;
      // Notify the polling thread to stop
      ::write (this-> _trigger.getWriteFd (), "c", 1);
      concurrency::join (this-> _th);
    }

    while (this-> _nbSubmitted != this-> _nbCompleted) { // && this-> _jobs.len () != 0) { // wait for the finish of already running tasks
      char c;
      ::read (this-> _trigger.getReadFd (), &c, 1);
    }
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

    for (auto & [it, sock] : this-> _openSockets) { // Delete all allocated sockets
      delete sock;
    }

    this-> _openSockets.clear ();
    this-> _context.close ();
    this-> _trigger.dispose ();
    this-> _runningThreads.clear ();
    this-> _closed.clear ();
  }

  TcpServer::~TcpServer () {
    this-> dispose ();
  }

}
