#include "free_list.hh"
#include <iostream>
#include <cstring>

namespace rd_utils::memory::cache {

  void free_list_create (free_list_instance * memory, uint32_t total_size) {
    memset (memory, 0, total_size);
    memory-> total_size = total_size - sizeof (free_list_instance);
    memory-> head = sizeof (free_list_instance);
    free_list_node * head = reinterpret_cast<free_list_node*> (reinterpret_cast <uint8_t*> (memory) + sizeof (free_list_instance));

    head-> size = total_size - sizeof (free_list_instance);
    head-> next = 0;

    /*
    //                       /---------------------\
    //                       |                     v
    // | total_size 4B | memory 8B | head 8B | -> | offset 4B | size 4B  | next 8B | ... |
    //                                 \-------------^
    */
  }

  bool free_list_find_best_fit (const free_list_instance * inst, uint32_t size, uint32_t & offset, uint32_t & prevOffset) {
    auto memory = reinterpret_cast <const uint8_t*> (inst);
    uint32_t nodeOffset = inst-> head;

    uint32_t prev = 0;
    auto min_found = inst-> total_size + 1;
    bool found = false;

    while (nodeOffset != 0) {
      auto node = reinterpret_cast <const free_list_node*> (memory + nodeOffset);
      if (node-> size >= size && (min_found > node-> size || !found)) {
        offset = nodeOffset;
        prevOffset = prev;
        min_found = node-> size;
        found = true;
      }

      prev = nodeOffset;
      nodeOffset = node-> next;
    }

    return found;
  }

  bool free_list_allocate_inner (free_list_instance * inst, uint32_t & size, uint32_t & offset) {
    uint8_t * memory = reinterpret_cast <uint8_t*> (inst);
    uint32_t prevOffset = 0, nodeOffset = 0;
    if (!free_list_find_best_fit (inst, size, nodeOffset, prevOffset)) { return false; }

    auto node = reinterpret_cast<free_list_node*> (memory + nodeOffset);
    free_list_node* previous = nullptr;
    if (prevOffset != 0) { previous = reinterpret_cast <free_list_node*> (memory + prevOffset); }

    offset = nodeOffset;
    auto restSize = (node-> size - size);
    if (restSize > sizeof (free_list_node)) { // create a new node at the given offset
      uint32_t newNodeOffset = nodeOffset + size;
      free_list_node * new_node = reinterpret_cast <free_list_node*> (memory + newNodeOffset);

      new_node-> size = restSize;
      new_node-> next = node-> next;

      if (previous) {
        previous-> next = newNodeOffset;
      } else {
        inst-> head = newNodeOffset;
      }

      return true;
    } else { // not enough size to create a node, use it all
      size = node-> size;
      if (previous) {
        previous-> next = node-> next;
      } else {
        inst-> head = node-> next;
      }

      return true;
    }
  }

  uint32_t free_list_real_size (uint32_t size) {
    uint32_t reqSize = size + sizeof (uint32_t);
    if (reqSize < sizeof (free_list_node)) reqSize = sizeof (free_list_node);
    return reqSize;
  }

  bool free_list_allocate (free_list_instance * inst, uint32_t size, uint32_t & offset) {
    uint8_t * memory = reinterpret_cast <uint8_t*> (inst);
    uint32_t reqSize = free_list_real_size (size);

    if (free_list_allocate_inner (inst, reqSize, offset)) {
      *reinterpret_cast<uint32_t*> (memory + offset) = reqSize;
      offset += sizeof (uint32_t);

      return true;
    }

    return false;
  }

  bool free_list_free_inner (free_list_instance * inst, uint32_t offset, uint32_t size) {
    uint8_t * memory = reinterpret_cast <uint8_t*> (inst);
    uint32_t nodeOffset = inst-> head, prevOffset = 0;
    free_list_node* prev = nullptr;

    if (nodeOffset == 0) {
      inst-> head = offset;
      auto head = reinterpret_cast <free_list_node*> (memory + offset);

      head-> size = size;
      head-> next = 0;
      return true;
    }

    while (nodeOffset != 0) {
      auto node = reinterpret_cast <free_list_node*> (memory + nodeOffset);
      if (nodeOffset + node-> size == offset) { // freeing the block just after the node
        node-> size += size;

        // the current free block is touching the next element
        if (node-> next == (nodeOffset + node-> size)) {
          auto next = reinterpret_cast <free_list_node*> (memory + node-> next);
          node-> size += next-> size; // fusionning them
          node-> next = next-> next;
        }

        return true;
      }

      if (nodeOffset > offset) { // we past the offset of the block to free
        auto newNodeOffset = offset;
        auto new_node = reinterpret_cast <free_list_node*> (memory + offset);
        new_node-> size = size;

        if (prev) { // between two nodes
          new_node-> next = nodeOffset;
          prev-> next = newNodeOffset;
        } else { // between the beginning and the head
          new_node-> next = nodeOffset;
          inst-> head = newNodeOffset;
        }

        // Touching new node and next
        if (new_node-> next != 0 && newNodeOffset + new_node-> size == new_node-> next) {
          auto next = reinterpret_cast<free_list_node*> (memory + new_node-> next);
          new_node-> size += next-> size;
          new_node-> next = next-> next;
        }

        if (prev && prevOffset + prev-> size == newNodeOffset) {
          auto next = reinterpret_cast<free_list_node*> (memory + new_node-> next);
          prev-> size += new_node-> size;
          prev-> next = next-> next;
        }

        return true;
      }

      // last node, but the offset is past it
      if (node-> next == 0 && nodeOffset + node-> size < offset) {
        auto newNodeOffset = offset;
        auto new_node = reinterpret_cast <free_list_node*> (memory + offset);

        new_node-> size = size;
        new_node-> next = 0;
        node-> next = newNodeOffset;

        return true;
      }

      prev = node;
      prevOffset = nodeOffset;
      nodeOffset = node-> next;
    }

    return false;
  }

  bool free_list_free (free_list_instance * inst, uint32_t offset) {
    uint8_t * memory = reinterpret_cast <uint8_t*> (inst);
    auto size = *reinterpret_cast<uint32_t*> (memory + (offset - sizeof (uint32_t)));
    auto ret = free_list_free_inner (inst, offset - sizeof (uint32_t), size);

    return ret;
  }

  bool free_list_empty (free_list_instance * inst) {
    if (inst-> head == sizeof (free_list_instance)) {
      auto head = reinterpret_cast <free_list_node*> (reinterpret_cast <uint8_t*> (inst) + inst-> head);
      return  head-> size == inst-> total_size;
    }
    return false;
  }

  uint32_t free_list_max_size (const free_list_instance * inst) {
    auto memory = reinterpret_cast <const uint8_t*> (inst);
    auto nodeOffset = inst-> head;
    uint32_t max = 0;
    while (nodeOffset != 0) {
      auto node = reinterpret_cast <const free_list_node *> (memory + nodeOffset);
      if (node-> size - sizeof (uint32_t) > max) max = node-> size - sizeof (uint32_t);
      nodeOffset = node-> next;
    }

    return max;
  }

}

std::ostream & operator<<(std::ostream & s, const rd_utils::memory::cache::free_list_instance &inst) {
  auto memory = reinterpret_cast <const uint8_t*> (&inst);
  s << "LIST{";
  int i = 0;
  auto nodeOffset = inst.head;
  while (nodeOffset != 0) {
    auto node = reinterpret_cast <const rd_utils::memory::cache::free_list_node *> (memory + nodeOffset);
    if (i != 0) { s << ", "; i += 1; }
    s << "(" << nodeOffset << "," << nodeOffset + node-> size << ")";
    nodeOffset = node-> next;
  }
  s << "}";
  return s;
}
