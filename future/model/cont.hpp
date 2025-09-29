#pragma once

#include "state.hpp"

namespace exe::future {

// clang-format off

template <typename C, typename V>
concept Continuation = requires(C cont, V v, State s) {
  cont.Continue(std::move(v), s);
};

// clang-format on

}  // namespace exe::future

