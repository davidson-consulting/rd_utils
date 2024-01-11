#pragma once

#include <rd_utils/net/stream.hh>
#include <vector>
#include <map>
#include <rd_utils/concurrency/taskpool.hh>

#include <sys/epoll.h>

namespace rd_utils::net {

  class TcpSession;

  /**
   * Connection pool
   */
  class TcpPool {

    // The server address
    SockAddrV4 _addr;

    // The number of active connection
    std::map <int, TcpStream*> _open;

    // The handles of the sockets
    std::map <uint64_t, int> _socketFds;

    // All the connection that are available
    concurrency::LockFreeMailbox <TcpStream*> _free;

    // mutex locked when updating connections
    concurrency::mutex _m;

    // signal emitted when a connection is released
    concurrency::semaphore _release;

    // The maximum number of opened connection
    int _max = 0;

    friend TcpSession;

  private :

    TcpPool (const TcpPool& );
    void operator=(const TcpPool&);

  public:

    /**
     * @params:
     *    - addr: the address of the server
     *    - max: the maximum number of connection in the pool
     */
    TcpPool (SockAddrV4 addr, int max);

    /**
     * Move ctor
     */
    TcpPool (TcpPool &&);

    /**
     * Move affectation
     */
    void operator=(TcpPool &&);

    /**
     * @returns: an avalaible stream
     */
    TcpSession get ();

    /**
     * Dispose all the opened connection
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~TcpPool ();

  private:

    /**
     * Release a connection from the pool
     */
    void release (TcpStream * stream);

  };


  class TcpSession {

    TcpStream * _stream;

    TcpPool * _context;

  private:

    TcpSession (const TcpSession &);
    void operator=(const TcpSession &);

  public:

    TcpSession (TcpStream * stream, TcpPool * context);

    TcpSession (TcpSession && other);

    void operator=(TcpSession && other);

    TcpStream* operator-> ();

    void dispose ();

    ~TcpSession ();

  };

}
