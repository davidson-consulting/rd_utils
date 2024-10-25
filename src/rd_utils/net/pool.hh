#pragma once

#include <rd_utils/net/stream.hh>
#include <vector>
#include <map>
#include <rd_utils/concurrency/taskpool.hh>
#include <memory>

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
    std::map <int, std::shared_ptr <TcpStream> > _open;

    // The handles of the sockets
    std::map <uint64_t, int> _socketFds;

    // All the connection that are available
    concurrency::Mailbox <std::shared_ptr <TcpStream> > _free;

    // mutex locked when updating connections
    concurrency::mutex _m;

    // signal emitted when a connection is released
    concurrency::semaphore _release;

    // The maximum number of opened connection
    uint32_t _max = 0;

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
     * @params:
     *    - timeout: -1 means wait until found, otherwise throw if not found before timeout
     * @returns: an avalaible stream
     */
    TcpSession get (float timeout = 5);


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
    void release (std::shared_ptr <TcpStream> stream);

  };



}
