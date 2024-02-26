#pragma once

#include <cstdlib>

struct garbage_collection_t {};
extern const garbage_collection_t GC;

void* operator new (size_t cbSize, const garbage_collection_t &);
