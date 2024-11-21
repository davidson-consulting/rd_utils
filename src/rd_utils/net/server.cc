#define LOG_LEVEL 10

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
    , _addr (0, 0)
    , _context (nullptr)
    , _th (0, nullptr)
    , _trigger (false)
  {}

  TcpServer::TcpServer (SockAddrV4 v4, int nbThreads) :
    _epoll_fd (0)
    , _addr (v4)
    , _context (nullptr)
    ,_nbThreads (getNbThreads (nbThreads))
    ,_th (0, nullptr)
    ,_trigger (true)
  {}

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          MASTER          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void TcpServer::start (void (*onSession)(std::shared_ptr <TcpStream>)) {
    if (this-> _started) throw std::runtime_error ("Already running");
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

    this-> _context = std::make_shared <TcpListener> (this-> _addr);
    this-> _context-> start ();
    
    this-> _trigger.setNonBlocking ();

    epoll_event event;
    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _trigger.ipipe ().getHandle ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event);

    event.events = EPOLLIN | EPOLLHUP;
    event.data.fd = this-> _context-> getHandle ();
    epoll_ctl (this-> _epoll_fd, EPOLL_CTL_ADD, this-> _context-> getHandle (), &event);
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          MAIN LOOP          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void TcpServer::pollMain (concurrency::Thread) {
    this-> _started = true;
    this-> _ready.post ();

    epoll_event event;
    while (this-> _started) {
      int event_count = epoll_wait (this-> _epoll_fd, &event, 1, -1);
      if (event_count == 0) {
        throw std::runtime_error ("Error while tcp waiting");
      }

      if (event.data.fd == this-> _context-> getHandle ()) { // -> New socket
        try {
          auto stream = this-> _context-> accept ();
          stream-> setSendTimeout (this-> _sendTimeout);
          stream-> setRecvTimeout (this-> _recvTimeout);

          this-> submit (stream);
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


  void TcpServer::submit (std::shared_ptr <TcpStream> stream) {
    this-> _jobs.send (stream);
    if (this-> _runningThreads.size () != (uint32_t) this-> _nbThreads) {
      this-> spawnThreads ();
    }

    this-> _nbSubmitted += 1;
    this-> _waitTask.post ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          WORKER          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void TcpServer::spawnThreads () {
    uint32_t start = this-> _runningThreads.size ();
    for (uint32_t i = start ; i < this-> _nbThreads ; i++) {
      auto th = spawn (this, &TcpServer::workerThread);
      this-> _ready.wait ();

      WITH_LOCK (this-> _triggerM) {
        this-> _runningThreads.emplace (th.id, th);
      }
    }
  }

  void TcpServer::workerThread (concurrency::Thread t) {
    this-> _ready.post ();

    for (;;) {
      this-> _waitTask.wait ();
      if (!this-> _started) break;

      for (;;) {
        std::shared_ptr <net::TcpStream> stream;
        if (this-> _jobs.receive (stream)) {
          this-> _onSession.emit (stream);
        } else {
          break;
        }
      }
    }

    WITH_LOCK (this-> _triggerM) {
      this-> _runningThreads.erase (t.id);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          GETTERS          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  int TcpServer::port () const {
    if (this-> _context == nullptr) return 0;
    return this-> _context-> port ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          SETTERS          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void TcpServer::setRecvTimeout (float timeout) {
    this-> _recvTimeout = timeout;
  }

  void TcpServer::setSendTimeout (float timeout) {
    this-> _sendTimeout = timeout;
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          CLOSING          =====================================
   * ====================================================================================================
   * ====================================================================================================
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
      // Notify the polling thread to stop
      WITH_LOCK (this-> _triggerM) {
        this-> _started = false;
        std::ignore = ::write (this-> _trigger.opipe ().getHandle (), "c", 1);
      }

      concurrency::join (this-> _th); // waiting for the main thread epolling
    }

    this-> _jobs.clear ();
    for (;;) {
      uint32_t size = 0;
      WITH_LOCK (this-> _triggerM) {
        size = this-> _runningThreads.size ();
      }

      if (size != 0) {
        this-> _waitTask.post ();
      } else break;
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

  bool TcpServer::dispose () {
    if (this-> isWorkerThread ()) return false;

    this-> waitAllCompletes ();
    if (this-> _epoll_fd != 0) { // clear epoll list
      ::close (this-> _epoll_fd);
      this-> _epoll_fd = 0;
    }

    this-> _context.reset ();
    this-> _trigger.dispose ();
    return true;
  }

  TcpServer::~TcpServer () {
    this-> dispose ();
  }

}
