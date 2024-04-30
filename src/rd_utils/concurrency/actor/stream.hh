#pragma once

#include <rd_utils/net/stream.hh>
#include <rd_utils/net/session.hh>
#include <rd_utils/utils/config/_.hh>
#include <rd_utils/utils/raw_parser.hh>
#include <memory>

namespace rd_utils::concurrency::actor {

  class ActorStream {
  private:

    net::TcpSession _input;
    net::TcpSession _output;
    bool _left; // left side is the one that asked for a stream

  private:

    ActorStream (const ActorStream &);
    void operator=(ActorStream&);

  public:

    ActorStream (net::TcpSession && input, net::TcpSession && output, bool left);

    bool next ();

    void stop ();

    void write (const utils::config::ConfigNode & node);

    std::shared_ptr <utils::config::ConfigNode> read ();

    ~ActorStream ();

  };


}
