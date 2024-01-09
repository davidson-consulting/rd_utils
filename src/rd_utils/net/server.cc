#include "server.hh"
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <rd_utils/utils/print.hh>


namespace rd_utils::net {

  TcpServer::TcpServer () :
    _context (SockAddrV4 (0, 0))
    , _trigger (false)
    , _epoll_fd (0)
    , _pool (0)
    , _th (0, nullptr)
  {}

  TcpServer::TcpServer (SockAddrV4 v4, int nbThreads) :
    _context (v4),
    _trigger (true),
    _pool (nbThreads),
    _th (0, nullptr)
  {}

  TcpServer::TcpServer (TcpServer && other) :
    _context (std::move (other._context))
    , _openSockets (std::move (other._openSockets))
    , _trigger (std::move (other._trigger))
    , _m (std::move (other._m))
    , _epoll_fd (other._epoll_fd)
    , _pool (std::move (other._pool))
    , _onSession (std::move (other._onSession))
    , _epoll_nb (other._epoll_nb)
    , _th (other._th)
    , _ready (std::move (other._ready))
    , _readySig (std::move (other._readySig))
  {
    other._epoll_fd = 0;
    other._epoll_nb = 0;
    other._th = concurrency::Thread (0, nullptr);
  }

  void TcpServer::operator=(TcpServer && other) {
    this-> dispose ();

    this-> _context = std::move (other._context);
    this-> _openSockets = std::move (other._openSockets);
    this-> _trigger = std::move (other._trigger);
    this-> _m = std::move (other._m);
    this-> _epoll_fd = other._epoll_fd;
    this-> _pool = std::move (other._pool);
    this-> _onSession = std::move (other._onSession);
    this-> _epoll_nb = other._epoll_nb;
    this-> _ready = std::move (other._ready);
    this-> _readySig = std::move (other._readySig);

    other._epoll_fd = 0;
    other._epoll_nb = 0;
  }

  void TcpServer::start (void (*onSession)(TcpSessionKind, TcpStream&)) {
    if (this-> _started) throw utils::Rd_UtilsError ("Already running");
    this-> _onSession.dispose ();
    this-> _onSession.connect (onSession);

    this-> _th = concurrency::spawn (this, &TcpServer::pollMain);
    WITH_LOCK (this-> _ready) {
      this-> _readySig.wait (this-> _ready);
    }
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

    this-> _epoll_nb = 2;
  }

  void TcpServer::pollMain (concurrency::Thread) {
    this-> _started = true;
    this-> configureEpoll ();
    this-> _readySig.signal ();

    epoll_event event;
    int event_count = 0;
    while (this-> _started) {
      WITH_LOCK (this-> _m) {
        event_count = epoll_wait (this-> _epoll_fd, &event, 1, -1);
      }

      if (event_count == 0) {
        throw utils::Rd_UtilsError ("Error while tcp waiting");
      }

      // New socket
      if (event.data.fd == this-> _context._sockfd) {
        auto stream = new TcpStream (std::move (this-> _context.accept ()));
        this-> _openSockets.emplace (stream-> getHandle (), stream);

        WITH_LOCK (this-> _m) {
          event.events = EPOLLIN | EPOLLONESHOT;
          event.data.fd = stream-> getHandle ();
          epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, stream-> getHandle (), &event);
        }

        this-> submit (TcpSessionKind::NEW, stream);
      }

      // on session close
      else if (event.data.fd == this-> _trigger.getReadFd ()) {
        char c;
        ::read (this-> _trigger.getReadFd (), &c, 1);
        usleep (100);
      }

      // Old client is writing
      else {
        WITH_LOCK (this-> _m) {
          auto it = this-> _openSockets.find (event.data.fd);
          this-> submit (TcpSessionKind::OLD, it-> second);
        }
      }
    }
  }

  int TcpServer::port () const {
    return this-> _context.port ();
  }

  int TcpServer::nbConnected () const {
    return this-> _openSockets.size ();
  }

  void TcpServer::submit (TcpSessionKind kind, TcpStream * stream) {
    this-> _pool.submit (this, &TcpServer::onSessionMain, kind, stream);
  }

  void TcpServer::onSessionMain (TcpSessionKind kind, TcpStream * stream) {
    this-> _onSession.emit (kind, *stream);
    this-> close (stream);
  }

  void TcpServer::close (TcpStream * stream) {
     // we need to trigger the epoll, because we are modifiying the epoll list
    ::write (this-> _trigger.getWriteFd (), "c", 1);
    WITH_LOCK (this-> _m) {
      if (!stream-> isOpen ()) {
        epoll_event event;
        epoll_ctl (this-> _epoll_fd, EPOLL_CTL_DEL, stream-> getHandle (), &event);
        this-> _epoll_nb += 1;
        this-> _openSockets.erase (stream-> getHandle ());
        stream-> close ();
        delete stream;
      } else {
        epoll_event event;
        event.events = EPOLLIN | EPOLLONESHOT;
        event.data.fd = stream-> getHandle ();
        epoll_ctl (this-> _epoll_fd, EPOLL_CTL_MOD, stream-> getHandle (), &event);

        this-> _epoll_nb += 1;
      }
    }
  }

  void TcpServer::join () {
    if (this-> _started) {
      concurrency::join (this-> _th);
    }
  }

  void TcpServer::stop () {
    this-> _started = false;
  }

  void TcpServer::dispose () {
    if (this-> _started) {
      this-> _started = false;
      // Notify the polling thread to stop
      ::write (this-> _trigger.getWriteFd (), "c", 1);
      concurrency::join (this-> _th);
    }

    this-> _pool.join ();
    if (this-> _epoll_fd != 0) {
      ::close (this-> _epoll_fd);
      this-> _epoll_fd = 0;
      this-> _epoll_nb = 0;
    }

    for (auto & [it, str] : this-> _openSockets) {
      delete str;
    }

    this-> _openSockets.clear ();
    this-> _context.close ();
    this-> _trigger.dispose ();
  }

  TcpServer::~TcpServer () {
    this-> dispose ();
  }

}
