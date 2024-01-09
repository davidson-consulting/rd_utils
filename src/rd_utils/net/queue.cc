#include "queue.hh"
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <rd_utils/utils/print.hh>


namespace rd_utils::net {

  TcpQueue::TcpQueue () :
    _context (nullptr)
    ,_trigger (false)
    ,_epoll_fd (0),
    _pool (0)
  {}

  TcpQueue::TcpQueue (TcpListener & context, int nbThreads, void (*onSession)(TcpSessionKind, TcpStream&)) :
    _context (&context),
    _trigger (true),
    _pool (nbThreads)
  {
    this-> _onSession.connect (onSession);
    this-> _epoll_fd = epoll_create1 (0);

    this-> _trigger.setNonBlocking ();

    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _trigger.getReadFd ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, this-> _trigger.getReadFd (), &event);

    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _context-> _sockfd;
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, this-> _context-> _sockfd, &event);

    this-> _epoll_nb = 2;
  }

  // TcpQueue::TcpQueue (TcpQueue && other) :
  //   _context (other._context)
  //   ,_maxSize (other._maxSize)
  //   ,_openSockets (std::move (other._openSockets))
  //   , _trigger (std::move (other._trigger))
  //   ,_m (std::move (other._m))
  //   ,_epoll_fd (other._epoll_fd)
  // {
  //   other._context = nullptr;
  //   other._epoll_fd = 0;
  // }

  // void TcpQueue::operator=(TcpQueue && other) {
  //   this-> dispose ();

  //   this-> _context = other._context;
  //   this-> _maxSize = other._maxSize;
  //   this-> _openSockets = std::move (other._openSockets);
  //   this-> _trigger = std::move (other._trigger);
  //   this-> _m = std::move (other._m);
  //   this-> _epoll_fd = other._epoll_fd;

  //   other._context = nullptr;
  //   other._epoll_fd = 0;
  // }

  void TcpQueue::poll () {
    epoll_event event;
    int event_count = 0;
    for (;;) {
      WITH_LOCK (this-> _m) {
        event_count = epoll_wait (this-> _epoll_fd, &event, 1, -1);
      }

      if (event_count == 0) {
        throw utils::Rd_UtilsError ("Error while tcp waiting");
      }

      // New socket
      if (event.data.fd == this-> _context-> _sockfd) {
        auto stream = new TcpStream (std::move (this-> _context-> accept ()));
        this-> _openSockets.emplace (stream-> getHandle (), stream);

        WITH_LOCK (this-> _m) {
          event.events = EPOLLIN | EPOLLONESHOT;
          event.data.fd = stream-> getHandle ();
          epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, stream-> getHandle (), &event);
        }

        return this-> submit (TcpSessionKind::NEW, stream);
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
          return this-> submit (TcpSessionKind::OLD, it-> second);
        }
      }
    }
  }

  int TcpQueue::len () const {
    return this-> _openSockets.size ();
  }


  void TcpQueue::submit (TcpSessionKind kind, TcpStream * stream) {
    this-> _pool.submit (this, &TcpQueue::onSessionMain, kind, stream);
  }

  void TcpQueue::onSessionMain (TcpSessionKind kind, TcpStream * stream) {
    this-> _onSession.emit (kind, *stream);
    this-> close (stream);
  }

  void TcpQueue::close (TcpStream * stream) {
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

  void TcpQueue::dispose () {
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
    this-> _context = nullptr;
    this-> _trigger.dispose ();
  }

  TcpQueue::~TcpQueue () {
    this-> dispose ();
  }

}
