#pragma once
#include <cstdint>
#include <iostream>

namespace rd_utils::memory::cache {

  struct free_list_node {
    uint32_t size;
    uint32_t next;
  };

  struct free_list_instance {
    uint32_t total_size;
    uint32_t head;
  };

  /**
   * Initialize a allocated memory segment to store a free list
   */
  void free_list_create (free_list_instance * memory, uint32_t total_size);

  /**
   * Tries to find a segment in which the size fits
   * @returns:
   *    - offset: the offset of the segment, if found
   *    - true, iif a segment was found
   *    - max_size: the remaining maximum allocable size (biggest remaining free block after allocation)
   */
  bool free_list_allocate (free_list_instance * inst, uint32_t size, uint32_t & offset);

  /**
   * @returns: the size that will be allocated to store an object of size size
   */
  uint32_t free_list_real_size (uint32_t size);

  /**
   * Free the segment of memory at offset
   * @params:
   *    - offset: the offset to free
   */
  bool free_list_free (free_list_instance * inst, uint32_t offset);

  /**
   * @returns: true if the list is empty
   */
  bool free_list_empty (free_list_instance * inst);

  /**
   * @returns: the size of the biggest free block in the list
   */
  uint32_t free_list_max_size (const free_list_instance * inst);

}


std::ostream & operator<<(std::ostream & os, const rd_utils::memory::cache::free_list_instance &inst);
