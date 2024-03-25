#pragma once


#include <rd_utils/net/_.hh>
#include <rd_utils/memory/cache/_.hh>

namespace rd_utils::memory::cache::remote {

#define WRITE_BUFFER_SIZE 1024 * 1024 * 1024

  enum class RepositoryProtocol : uint32_t {
    STORE = 1,
    EXISTS,
    LOAD,
    ERASE,
    REGISTER,
    CLOSE
  };

  /**
   * Repository for remote access to blocks
   */
  class Repository {
  private :

    concurrency::mutex _m;

    // The maximum number of blocks that can be loaded in memory at the same time
    uint32_t _nbBlocks;

    // The size of a block
    uint32_t _blockSize;

    // The address of the repository
    net::SockAddrV4 _addr;

    // Server used for pages input/output
    net::TcpServer _server;

    // The list of loaded blocks
    std::unordered_map <uint64_t, uint8_t*> _loaded;

    // The persister to store blocks to disk
    BlockPersister * _persister;

    // The last user id
    uint32_t _userId = 0;

  public:

    /**
     * @params:
     *    - addr: the listening address
     *    - nbBlocks: the maximum number of blocks the repository can store into RAM
     *    - blockSize: the size of a block
     *    - maxCon: the maximum number of clients that can be connected to the repository at the same time
     */
    Repository (net::SockAddrV4 addr, uint32_t nbBlocks, uint32_t blockSize, int maxCon = -1);

    /**
     * Start the repository, now ready for incoming connections and requests
     */
    void start ();

    /**
     * Stop the repository
     * @warning: all blocks managed by the repo will be lost
     */
    void stop ();

    /**
     * Wait for the end of the repository (infinite until this-> stop is called)
     */
    void join ();

    /**
     * clear all loaded and in disk blocks
     */
    void dispose ();

    /**
     * Clear all blocks
     */
    ~Repository ();

  private:

    /**
     * On client connection or request
     * @params:
     *    - kind: new connection, or old
     *    - stream: the tcp stream of the client
     */
    void onSession (net::TcpSessionKind, net::TcpStream&);

    /**
     * Store a block received from the stream
     */
    void store (net::TcpStream&);

    /**
     * Check if the block exists
     */
    void exists (net::TcpStream&);

    /**
     * Load a block received and send it to the stream
     */
    void load (net::TcpStream&);

    /**
     * Erase an allocated block
     */
    void erase (net::TcpStream&);

    /**
     * Evict a block in the list of loaded blocks
     */
    uint8_t* evict ();

    /**
     * Write a block to tcp stream
     */
    void write (net::TcpStream&, uint8_t*mem);

    /**
     * Read a block from tcp stream
     */
    void read (net::TcpStream&, uint8_t*mem);

  };

}
