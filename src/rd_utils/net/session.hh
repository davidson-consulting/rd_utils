#pragma once

#include <memory>

namespace rd_utils::net {

  class TcpStream;
  class TcpPool;
  class TcpServer;

  /**
   * A tcp session is used by TcpServer and TcpPool
   * It contains a TcpStream that is provided by a server or a pool
   * The objective is to make sure TcpStream are correctly returned to the server when the session ends
   */
  class TcpSession {

    std::shared_ptr <TcpStream> _stream;

    void * _context;

    bool _server;

    // If true the session was never used
    bool _ignored;

  public :

    /**
     * @params:
     *    - stream: the stream managed by the session
     *    - context: the server managing the session
     */
    TcpSession (std::shared_ptr <TcpStream> stream, TcpServer * context);

    /**
     * @params:
     *    - stream: the stream managed by the session
     *    - context: the pool managing the session
     */
    TcpSession (std::shared_ptr <TcpStream> stream, TcpPool * context);

    friend net::TcpPool;
    friend net::TcpServer;

  public:

    /**
     * Move ctor
     */
    TcpSession (TcpSession && other);

    /**
     * Move affectation
     */
    void operator=(TcpSession && other);

    TcpSession (const TcpSession &) = delete;
    void operator=(const TcpSession &) = delete;

    /**
     * Access the inner stream
     * @example:
     * ======
     * void onSession (TcpSession & session) {
     *    auto i = session-> receiveI32 ();
     *
     * }
     * ======
     */
    std::shared_ptr <TcpStream> operator-> ();

    /**
     * Access the inner stream
     * @example:
     * ========
     * void onSession (TcpSession & session) {
     *    readStr (*session);
     * }
     *
     * void readStr (net::TcpStream & str) { // ... }
     * ========
     */
    TcpStream& operator* ();

    /**
     * Dispose the session, and return the stream to the pool/server
     */
    void dispose ();

    /**
     * @cf: this-> dispose ()
     */
    ~TcpSession ();

  };


}
