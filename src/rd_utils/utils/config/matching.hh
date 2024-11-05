#pragma once

#include <memory>

#define match(X)                                \
  auto ref = &X;

#define of(X, x)                                            \
  if (auto x = dynamic_cast<const X*>(ref) ; x != NULL) {

#define elof(X, x)                              \
  } else of (X, x)

#define fo }                                    \

#define elfo } else                             \

#define CONF_LET(A, B, C)                         \
  auto A = C;                                     \
  try {                                           \
    A = B;                                        \
  } catch (...) {}

namespace rd_utils::utils::config {


}
