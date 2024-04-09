#include "persist.hh"
#include <iostream>
#include <rd_utils/utils/base64.hh>
#include <rd_utils/utils/log.hh>
#include "repo.hh"

namespace rd_utils::memory::cache::remote {

  BlockPersister::BlockPersister () {}

  void BlockPersister::printInfo () const {
    std::cout << "Load " << this-> _nbLoaded << " (" << this-> _loadElapsed << "), Save " << this-> _nbSaved << "(" << this-> _saveElapsed << ")\n";
  }

  void BlockPersister::getInfo (uint64_t & nbWrites, double & writeTime, uint64_t & nbReads, double & readTime) const {
    nbWrites = this-> _nbSaved;
    nbReads = this-> _nbLoaded;

    readTime = this-> _loadElapsed;
    writeTime = this-> _saveElapsed;
  }

  BlockPersister::~BlockPersister () {}

  /***
   * =====================================================================
   * =====================================================================
   * =========================   LOCAL PERSIST   =========================
   * =====================================================================
   * =====================================================================
   **/

  LocalPersister::LocalPersister (const std::string & path) :
    _path (path)
  {
    this-> _buffer = new char [255];
  }

  bool LocalPersister::exists (uint64_t addr) {
    this-> _nbLoaded += 1;
    int nb = snprintf (this-> _buffer, 255, "%s%ld", this-> _path.c_str (), addr);
    this-> _buffer [nb] = '\0';

    return utils::file_exists (this-> _buffer);
  }

  void LocalPersister::load (uint64_t addr, uint8_t * memory, uint64_t size) {
    LOG_INFO ("LOADING block : ", addr);

    this-> _nbLoaded += 1;
    int nb = snprintf (this-> _buffer, 255, "%s%ld", this-> _path.c_str (), addr);
    this-> _buffer [nb] = '\0';

    std::cout << this-> _buffer << std::endl;

    concurrency::timer t;
    auto file = fopen (this-> _buffer, "r");
    if (file == nullptr) {
      throw std::runtime_error ("???");
    }
    fread (memory, size, 1, file);
    fclose (file);
    this-> _loadElapsed += t.time_since_start ();
    remove (this-> _buffer);
  }

  void LocalPersister::save (uint64_t addr, uint8_t * memory, uint64_t size) {
    LOG_INFO ("STORING block : ", addr);

    this-> _nbSaved += 1;
    int nb = snprintf (this-> _buffer, 255, "%s%ld", this-> _path.c_str (), addr);
    this-> _buffer [nb] = '\0';

    concurrency::timer t;
    auto file = fopen (this-> _buffer, "w");
    fwrite (memory, size, 1, file);
    this-> _saveElapsed += t.time_since_start ();

    fclose (file);
  }

  void LocalPersister::erase (uint64_t addr) {
    LOG_INFO ("ERASE block : ", addr);

    int nb = snprintf (this-> _buffer, 255, "%s%ld", this-> _path.c_str (), addr);
    this-> _buffer [nb] = '\0';

    remove (this-> _buffer);
    fflush (nullptr);
  }

  LocalPersister::~LocalPersister () {
    delete [] this-> _buffer;
  }

  /***
   * =====================================================================
   * =====================================================================
   * =========================   REMOTE PERSIST   ========================
   * =====================================================================
   * =====================================================================
   **/


  RemotePersister::RemotePersister (net::SockAddrV4 addr) :
    _addr (addr)
  {
    net::TcpStream r (this-> _addr);
    r.connect ();

    r.sendInt ((uint64_t) RepositoryProtocol::REGISTER);
    this-> _clientId = r.receiveInt ();

    r.close ();
  }

  bool RemotePersister::exists (uint64_t addr) {
    net::TcpStream r (this-> _addr);
    r.connect ();

    r.sendInt ((uint64_t) RepositoryProtocol::EXISTS);
    r.sendInt (this-> _clientId);
    r.sendInt (addr);
    auto result = r.receiveInt ();

    r.close ();

    return result == 1;
  }

  void RemotePersister::load (uint64_t addr, uint8_t * memory, uint64_t size) {
    concurrency::timer t;
    net::TcpStream r (this-> _addr);
    r.connect ();

    r.sendInt ((uint64_t) RepositoryProtocol::LOAD);
    r.sendInt (this-> _clientId);
    r.sendInt (addr);

    this-> read (r, memory, size);
    this-> _loadElapsed += t.time_since_start ();
    this-> _nbLoaded += 1;

    r.close ();
  }

  void RemotePersister::save (uint64_t addr, uint8_t * memory, uint64_t size) {
    concurrency::timer t;

    net::TcpStream r (this-> _addr);
    r.connect ();

    r.sendInt ((uint64_t) RepositoryProtocol::STORE);
    r.sendInt (this-> _clientId);
    r.sendInt (addr);

    this-> write (r, memory, size);
    this-> _saveElapsed += t.time_since_start ();
    this-> _nbSaved += 1;

    r.close ();
  }

  void RemotePersister::erase (uint64_t addr) {
    net::TcpStream r (this-> _addr);
    r.connect ();

    r.sendInt ((uint64_t) RepositoryProtocol::ERASE);
    r.sendInt (this-> _clientId);
    r.sendInt (addr);
    r.close ();
  }

  void RemotePersister::write (net::TcpStream & str, uint8_t * mem, uint64_t size) {
    auto handle = str.getHandle ();
    auto rest = size;

    LOG_INFO ("WritingBlock : ", "=>", rest);
    if (rest != 0) {
      uint32_t val = 0;
      do {
        auto val_ = ::send (handle, &mem[val], rest - val, 0);
        val += val_;
      } while (val != rest);
    }
  }

  void RemotePersister::read (net::TcpStream & str, uint8_t * mem, uint64_t size) {
    auto handle = str.getHandle ();
    auto rest = size; // % WRITE_BUFFER_SIZE;
    LOG_INFO ("Reading Block : ", "=>", rest);

    if (rest != 0) {
      uint32_t val = 0;
      do {
        auto val_ = ::recv (handle, &mem[val], rest - val, MSG_WAITALL);
        val += val_;
      } while (val != rest);
    }
  }

  RemotePersister::~RemotePersister () {
    std::cout << "Closing socket ?" << std::endl;
    net::TcpStream r (this-> _addr);
    r.sendInt ((uint32_t) RepositoryProtocol::CLOSE);
    r.close ();
  }

}
