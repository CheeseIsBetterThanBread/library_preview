#pragma once

#include "cont.hpp"
#include "comp.hpp"

namespace exe::future {

// Continuation

template <typename V>
struct Demand {
  void Continue(V, State) {
    // No-op
  }
};

// Thunk

template <typename T>
concept Thunk = requires(T thunk, Demand<typename T::ValueType> demand) {
  typename T::ValueType;

  { thunk.Materialize(demand) } -> Computation;
};

}  // namespace exe::future

