
#ifndef __PROJECT__
#define __PROJECT__ "TCPSESSION"
#endif

#include <rd_utils/utils/_.hh>
#include <rd_utils/net/_.hh>
#include "session.hh"
#include "server.hh"
#include "pool.hh"

namespace rd_utils::net {

  TcpSession::TcpSession (std::shared_ptr <TcpStream> stream, TcpServer * context) :
    _stream (stream)
    , _context (static_cast<void*> (context))
    , _server (true)
  {}

  TcpSession::TcpSession (std::shared_ptr <TcpStream> stream, TcpPool * context) :
    _stream (stream)
    , _context (static_cast<void*> (context))
    , _server (false)
    , _ignored (true)
  {}

  TcpSession::TcpSession (TcpSession && other) :
    _stream (other._stream)
    , _context (other._context)
    , _server (other._server)
    , _ignored (other._ignored)
  {
    other._stream = nullptr;
    other._context = nullptr;
    other._ignored = false;
  }

  void TcpSession::operator= (TcpSession && other) {
    this-> dispose ();
    this-> _stream = other._stream;
    this-> _context = other._context;
    this-> _server = other._server;
    this-> _ignored = other._ignored;

    other._stream = nullptr;
    other._context = nullptr;
    other._ignored = false;
  }

  void TcpSession::dispose () {
    if (this-> _context != nullptr) {
      if (this-> _server) {
        if (this-> _ignored) this-> _stream-> close ();
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
    this-> _ignored = false;
    return this-> _stream;
  }

  TcpStream& TcpSession::operator* () {
    this-> _ignored = false;
    return *this-> _stream;
  }

}
