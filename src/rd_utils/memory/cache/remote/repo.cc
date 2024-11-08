#include "repo.hh"

namespace rd_utils::memory::cache::remote {

  Repository::Repository (net::SockAddrV4 addr, uint32_t nbBlocks, uint32_t blockSize) :
    _nbBlocks (nbBlocks)
    , _blockSize (blockSize)
    , _addr (addr)
    , _server (addr, 1)
  {
    this-> _persister = new LocalPersister ();
  }

  void Repository::start () {
    this-> _server.start (this, &Repository::onSession);
  }

  void Repository::stop () {
    this-> _server.stop ();
  }

  void Repository::join () {
    this-> _server.join ();
  }

  void Repository::dispose () {
    this-> _server.dispose ();
  }

  Repository::~Repository () {
    this-> dispose ();
  }

  void Repository::onSession (std::shared_ptr <net::TcpStream> str) {
    auto proto = str-> receiveU32 ();
    switch ((RepositoryProtocol) proto) {
    case RepositoryProtocol::EXISTS :
      this-> exists (*str);
      break;
    case RepositoryProtocol::STORE:
      this-> store (*str);
      break;
    case RepositoryProtocol::LOAD:
      this-> load (*str);
      break;
    case RepositoryProtocol::ERASE:
      this-> erase (*str);
      break;
    case RepositoryProtocol::REGISTER:
      LOG_INFO ("New client");
      this-> _userId ++;
      str-> sendU32 (this-> _userId);
      break;
    default :
      break;
    }

    str-> close ();
  }

  void Repository::exists (net::TcpStream & str) {
    WITH_LOCK (this-> _m) {
      uint64_t uid = str.receiveU32 ();
      uint64_t blid = str.receiveU32 ();
      auto p = ((uint64_t) (uid << 32)) | blid;

      auto memory = this-> _loaded.find (p);
      if (memory == this-> _loaded.end ()) {
        str.sendU32 (this-> _persister-> exists (p) ? 1 : 0);
      } else {
        str.sendU32 (1);
      }
    }
  }

  void Repository::store (net::TcpStream & str) {
    WITH_LOCK (this-> _m) {
      uint64_t uid = str.receiveU32 ();
      uint64_t blid = str.receiveU32 ();
      auto p = ((uint64_t) (uid << 32)) | blid;

      auto memory = this-> _loaded.find (p);
      uint8_t * mem = nullptr;
      if (memory == this-> _loaded.end ()) {
        if (this-> _loaded.size () == this-> _nbBlocks) {
          mem = this-> evict ();
        }

        if (mem == nullptr) {
          mem = new uint8_t [this-> _blockSize];
        }

        this-> _loaded.emplace (p, mem);
      } else {
        mem = memory-> second;
      }

      this-> read (str, mem);
    }
  }

  uint8_t* Repository::evict () {
    if (this-> _loaded.size () != 0) {
      auto fst = this-> _loaded.begin ();
      this-> _persister-> save (fst-> first, fst-> second, this-> _blockSize);
      auto ret = fst-> second;
      this-> _loaded.erase (fst-> first);

      return ret;
    } else {
      return nullptr;
    }
  }

  void Repository::load (net::TcpStream & str) {
    WITH_LOCK (this-> _m) {
      concurrency::timer t;
      uint64_t uid = str.receiveU32 ();
      uint64_t blid = str.receiveU32 ();
      auto p = ((uint64_t) (uid << 32)) | blid;

      auto memory = this-> _loaded.find (p);
      uint8_t * mem = nullptr;
      if (memory == this-> _loaded.end ()) {
        if (this-> _loaded.size () == this-> _nbBlocks) {
          mem = this-> evict ();
        }

        if (mem == nullptr) {
          mem = new uint8_t [this-> _blockSize];
        }

        this-> _loaded.emplace (p, mem);
        this-> _persister-> load (p, mem, this-> _blockSize);
      } else {
        mem = memory-> second;
      }

      this-> write (str, mem);
      delete [] mem;
      this-> _loaded.erase (p);
    }
  }

  void Repository::erase (net::TcpStream & str) {
    WITH_LOCK (this-> _m) {
      uint64_t uid = str.receiveU32 ();
      uint64_t blid = str.receiveU32 ();
      auto p = ((uint64_t) (uid << 32)) | blid;

      auto memory = this-> _loaded.find (p);
      if (memory != this-> _loaded.end ()) {
        delete[] memory-> second;
        this-> _loaded.erase (p);
      } else {
        this-> _persister-> erase (p);
      }
    }
  }

  void Repository::read (net::TcpStream & stream, uint8_t * mem) {
    auto handle = stream.getHandle ();
    auto rest = this-> _blockSize;

    if (rest != 0) {
      uint32_t val = 0;
      do {
        auto val_ = ::recv (handle, &mem[val], rest - val, MSG_WAITALL);
        val += val_;
      } while (val != rest);
    }
  }

  void Repository::write (net::TcpStream & stream, uint8_t * mem) {
    auto handle = stream.getHandle ();
    auto rest = this-> _blockSize;

    if (rest != 0) {
      uint32_t val = 0;
      do {
        auto val_ = ::send (handle, &mem[val], rest - val, 0);
        val += val_;
      } while (val != rest);
    }
  }

}
