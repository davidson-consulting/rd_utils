
#ifndef __PROJECT__
#define __PROJECT__ "TCPSESSION"
#endif

#include <rd_utils/utils/_.hh>
#include "session.hh"
#include "server.hh"
#include "pool.hh"

namespace rd_utils::net {

  TcpSession::TcpSession (std::shared_ptr <TcpStream> stream, void * context, bool server) :
    _stream (stream)
    , _context (context)
    , _server (server)
  {}

  TcpSession::TcpSession (TcpSession && other) :
    _stream (other._stream)
    , _context (other._context)
    , _server (other._server)
  {
    other._stream = nullptr;
    other._context = nullptr;
  }

  void TcpSession::operator= (TcpSession && other) {
    this-> dispose ();
    this-> _stream = other._stream;
    this-> _context = other._context;
    this-> _server = other._server;

    other._stream = nullptr;
    other._context = nullptr;
  }

  void TcpSession::dispose () {
    if (this-> _context != nullptr) {
      LOG_DEBUG ("Disposing session : ", this-> _stream-> getHandle (), " ", this-> _server);
      // std::cout << "Freeing " << this-> _stream-> getHandle () << std::endl;
      if (this-> _server) {
        reinterpret_cast <TcpServer*> (this-> _context)-> release (this-> _stream);
      } else {
        reinterpret_cast <TcpPool*> (this-> _context)-> release (this-> _stream);
      }

      this-> _context = nullptr;
      this-> _stream = nullptr;
    }
  }

  TcpSession::~TcpSession () {
    this-> dispose ();
  }

  std::shared_ptr <TcpStream> TcpSession::operator-> () {
    return this-> _stream;
  }

  TcpStream& TcpSession::operator* () {
    return *this-> _stream;
  }

}
