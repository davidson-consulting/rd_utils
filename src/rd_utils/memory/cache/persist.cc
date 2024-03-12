#include "persist.hh"
#include <iostream>
#include <rd_utils/utils/base64.hh>
#include <rd_utils/utils/log.hh>

#include "free_list.hh"

namespace rd_utils::memory::cache {

  BlockPersister::BlockPersister (const std::string & path) :
    _path (path)
  {
    this-> _buffer = new char [255];
  }

  bool BlockPersister::load (uint64_t addr, uint8_t * memory, uint64_t size) {
    this-> _nbLoaded += 1;
    int nb = snprintf (this-> _buffer, 255, "%s%ld", this-> _path.c_str (), addr);
    this-> _buffer [nb] = '\0';

    concurrency::timer t;
    auto file = fopen (this-> _buffer, "r");
    if (file != nullptr) {
      auto ignore = fread (memory, size, 1, file);
      fclose (file);
      this-> _loadElapsed += t.time_since_start ();
      // remove (buffer);
      return true;
    } else {
      return false;
    }
  }

  void BlockPersister::save (uint64_t addr, uint8_t * memory, uint64_t size) {
    this-> _nbSaved += 1;
    int nb = snprintf (this-> _buffer, 255, "%s%ld", this-> _path.c_str (), addr);
    this-> _buffer [nb] = '\0';

    concurrency::timer t;
    auto file = fopen (this-> _buffer, "w");
    fwrite (memory, size, 1, file);
    this-> _saveElapsed += t.time_since_start ();

    fclose (file);
  }

  void BlockPersister::erase (uint64_t addr) {
    int nb = snprintf (this-> _buffer, 255, "%s%ld", this-> _path.c_str (), addr);
    this-> _buffer [nb] = '\0';

    remove (this-> _buffer);
    fflush (nullptr);
  }

  void BlockPersister::printInfo () const {
    std::cout << "Load " << this-> _nbLoaded << " (" << this-> _loadElapsed << "), Save " << this-> _nbSaved << "(" << this-> _saveElapsed << ")\n";
  }

  BlockPersister::~BlockPersister () {
    delete [] this-> _buffer;
  }
}
