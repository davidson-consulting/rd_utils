#include "persist.hh"
#include <iostream>
#include <rd_utils/utils/base64.hh>

namespace rd_utils::memory::cache {


  BlockPersister::BlockPersister (const std::string & path) :
    _path (path)
  {}

  void BlockPersister::load (uint64_t addr, uint8_t * memory, uint64_t size) {
    std::cout << "Loading : " << addr << std::endl;
    char buffer [255];
    int nb = snprintf (buffer, sizeof (buffer), "%s%ld", this-> _path.c_str (), addr);
    buffer [nb] = '\0';

    auto file = fopen (buffer, "r");
    if (file != nullptr) {
      fread (memory, size, 1, file);
      fclose (file);
    }
    remove (buffer);
  }

  void BlockPersister::save (uint64_t addr, uint8_t * memory, uint64_t size) {
    char buffer [255];
    int nb = snprintf (buffer, sizeof (buffer), "%s%ld", this-> _path.c_str (), addr);
    buffer [nb] = '\0';

    auto file = fopen (buffer, "w");
    fwrite (memory, size, 1, file);

    fclose (file);
  }

  void BlockPersister::erase (uint64_t addr) {
    std::cout << "Erasing : " << addr << std::endl;
    char buffer [255];
    int nb = snprintf (buffer, sizeof (buffer), "%s%ld", this-> _path.c_str (), addr);
    buffer [nb] = '\0';

    remove (buffer);
  }

}
