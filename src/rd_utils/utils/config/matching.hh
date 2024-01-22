#pragma once
#include <memory>

#define match(X)                                \
  auto ref = X;

#define of(X, x)                                            \
  if (auto x = dynamic_pointer_cast<X>(ref) ; x != NULL) {

#define elof(X, x)                              \
  } else of (X, x)

#define fo }                                    \

#define elfo } else                             \
