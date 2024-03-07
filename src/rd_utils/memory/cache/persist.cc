#include "persist.hh"
#include <iostream>
#include <rd_utils/utils/base64.hh>
#include <rd_utils/utils/log.hh>

#include "free_list.hh"

namespace rd_utils::memory::cache {


  BlockPersister::BlockPersister (const std::string & path) :
    _path (path)
  {}

  bool BlockPersister::load (uint64_t addr, uint8_t * memory, uint64_t size) {
    //std::cout << "Load : " << addr << std::endl;

    this-> _nbLoaded += 1;
    char buffer [255];
    int nb = snprintf (buffer, sizeof (buffer), "%s%ld", this-> _path.c_str (), addr);
    buffer [nb] = '\0';

    auto file = fopen (buffer, "r");
    if (file != nullptr) {
      auto ignore = fread (memory, size, 1, file);
      fclose (file);
      remove (buffer);

      return true;
    } else {
      return false;
    }
  }

  void BlockPersister::save (uint64_t addr, uint8_t * memory, uint64_t size) {
    // std::cout << "Save : " << addr << std::endl;

    this-> _nbSaved += 1;
    char buffer [255];
    int nb = snprintf (buffer, sizeof (buffer), "%s%ld", this-> _path.c_str (), addr);
    buffer [nb] = '\0';

    auto file = fopen (buffer, "w");
    fwrite (memory, size, 1, file);

    fclose (file);
  }

  void BlockPersister::erase (uint64_t addr) {
    char buffer [255];
    int nb = snprintf (buffer, sizeof (buffer), "%s%ld", this-> _path.c_str (), addr);
    buffer [nb] = '\0';

    remove (buffer);
  }

  void BlockPersister::printInfo () const {
    LOG_INFO ("Save : ", this-> _nbSaved, ", Load ", this-> _nbLoaded);
  }

}
