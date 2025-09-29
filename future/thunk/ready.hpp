#pragma once

#include <exe/future/model/thunk.hpp>

namespace exe::future {

namespace thunk {

template <typename V>
struct [[nodiscard]] Ready {
  V value;

  explicit Ready(V v)
      : value(std::move(v)) {
  }

  // Non-copyable
  Ready(const Ready&) = delete;
  Ready& operator=(const Ready&) = delete;

  Ready(Ready&&) = default;

  // Computation
  template <Continuation<V> Consumer>
  struct Compute {
    V value;
    Consumer consumer;

    void Start(runtime::View rt) {
      consumer.Continue(std::move(value), State{rt});
    }
  };

  // Thunk

  using ValueType = V;

  template <Continuation<V> Consumer>
  Computation auto Materialize(Consumer&& c) {
    return Compute<Consumer>{std::move(value), std::forward<Consumer>(c)};
  }
};

}  // namespace thunk

}  // namespace exe::future

