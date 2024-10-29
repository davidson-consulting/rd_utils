
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
    , _pool (nullptr)
    , _server (context)
    , _ignored (true)
  {}

  TcpSession::TcpSession (std::shared_ptr <TcpStream> stream, TcpPool * context) :
    _stream (stream)
    , _pool (context)
    , _server (nullptr)
    , _ignored (true)
  {}

  TcpSession::TcpSession (TcpSession && other) :
    _stream (other._stream)
    , _pool (other._pool)
    , _server (other._server)
    , _ignored (other._ignored)
  {
    other._stream = nullptr;
    other._pool = nullptr;
    other._server = nullptr;
    other._ignored = false;
  }

  void TcpSession::operator= (TcpSession && other) {
    this-> dispose ();
    this-> _stream = other._stream;
    this-> _pool = other._pool;
    this-> _server = other._server;
    this-> _ignored = other._ignored;

    other._stream = nullptr;
    other._server = nullptr;
    other._pool = nullptr;
    other._ignored = false;
  }

  void TcpSession::dispose () {
    if (this-> _stream != nullptr) {
      if (this-> _server != nullptr) {
        if (this-> _ignored) this-> _stream-> close ();
        this-> _server-> release (this-> _stream);
      } else if (this-> _pool != nullptr) {
        this-> _pool-> release (this-> _stream);
      }

      this-> _server = nullptr;
      this-> _pool = nullptr;
      this-> _stream = nullptr;
    }
  }

  TcpSession::~TcpSession () {
    this-> dispose ();
  }

  void TcpSession::kill () {
    if (this-> _stream != nullptr) {
      this-> _stream-> close ();
      this-> dispose ();
    }
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
