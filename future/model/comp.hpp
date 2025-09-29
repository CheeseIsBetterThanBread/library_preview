#pragma once

#include <exe/runtime/view.hpp>

namespace exe::future {

// clang-format off

template <typename C>
concept Computation = requires(C comp, runtime::View rt) {
  comp.Start(rt);
};

// clang-format on

}  // namespace exe::future

