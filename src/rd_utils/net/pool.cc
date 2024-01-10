#include "pool.hh"


namespace rd_utils::net {

  TcpSession::TcpSession (TcpStream * stream, TcpPool * context) :
    _stream (stream)
    , _context (context)
  {}

  TcpSession::TcpSession (TcpSession && other) :
    _stream (other._stream)
    , _context (other._context)
  {
    other._stream = nullptr;
    other._context = nullptr;
  }

  void TcpSession::operator= (TcpSession && other) {
    this-> dispose ();
    this-> _stream = other._stream;
    this-> _context = other._context;

    other._stream = nullptr;
    other._context = nullptr;
  }

  void TcpSession::dispose () {
    if (this-> _context != nullptr) {
      this-> _context-> release (this-> _stream);
      this-> _context = nullptr;
      this-> _stream = nullptr;
    }
  }

  TcpSession::~TcpSession () {
    this-> dispose ();
  }

  TcpStream* TcpSession::operator-> () {
    return this-> _stream;
  }

  TcpPool::TcpPool (SockAddrV4 addr, int max) :
    _addr (addr)
    , _max (max)
  {}

  TcpSession TcpPool::get () {
    auto conn = this-> _free.receive ();
    if (conn.has_value ()) {
      return TcpSession (*conn, this);
    }

    if (this-> _open.size () < this-> _max) {
      TcpStream * s = new TcpStream (this-> _addr);
      s-> connect ();
      this-> _open.emplace (s-> getHandle (), s);
      return TcpSession (s, this);
    } else {
      WITH_LOCK (this-> _release) {
        this-> _releasedSig.wait (this-> _release);
      }

      return this-> get ();
    }
  }

  void TcpPool::release (TcpStream * s) {
    this-> _free.send (s);
    this-> _releasedSig.signal ();
  }

  void TcpPool::dispose () {
    this-> _free.clear ();
    for (auto & [it, sock] : this-> _open) {
      delete sock;
    }

    this-> _open.clear ();
  }

  TcpPool::~TcpPool () {
    this-> dispose ();
  }


}
