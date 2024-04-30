#pragma once

#include <rd_utils/net/stream.hh>
#include <vector>
#include <map>
#include <rd_utils/concurrency/taskpool.hh>
#include <memory>

namespace rd_utils::net {

  class TcpSession {

    std::shared_ptr <TcpStream> _stream;

    void * _context;

    bool _server;

  private:

    TcpSession (const TcpSession &);
    void operator=(const TcpSession &);

  public:

    TcpSession (std::shared_ptr <TcpStream> stream, void * context, bool server);

    TcpSession (TcpSession && other);

    void operator=(TcpSession && other);

    std::shared_ptr <TcpStream> operator-> ();

    TcpStream& operator* ();

    void dispose ();

    ~TcpSession ();

  };


}
