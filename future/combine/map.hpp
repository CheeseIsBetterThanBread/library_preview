#pragma once

#include <type_traits>
#include <utility>  // std::move

#include <exe/future/type/future.hpp>
#include <exe/future/syntax/pipe.hpp>
#include <exe/future/trait/value_of.hpp>

#include <exe/future/model/thunk.hpp>

namespace exe::future {

namespace thunk {

template <Thunk Producer, typename F>
struct [[nodiscard]] Map {
  Producer producer;
  F user;

  explicit Map(Producer producer, F user)
      : producer(std::move(producer)),
        user(std::move(user)) {
  }

  // Non-copyable
  Map(const Map& other) = delete;
  Map& operator=(const Map& other) = delete;

  Map(Map&&) = default;

  using InputType = trait::ValueOf<Producer>;
  using OutputType = std::invoke_result_t<F, InputType>;

  // Thunk
  using ValueType = OutputType;

  template <Continuation<OutputType> Consumer>
  Computation auto Materialize(Consumer c) {
    return producer.Materialize(
        RunMap<Consumer>{std::move(user), std::move(c)});
  }

  // Continuation
  template <Continuation<OutputType> Consumer>
  struct RunMap {
    F user;
    Consumer consumer;

    explicit RunMap(F user, Consumer consumer)
        : user(std::move(user)),
          consumer(std::move(consumer)) {
    }

    void Continue(InputType value, State s) {
      consumer.Continue(user(std::move(value)), s);
    }
  };
};

}  // namespace thunk

namespace pipe {

template <typename F>
struct [[nodiscard]] Map {
  F user;

  explicit Map(F u)
      : user(std::move(u)) {
  }

  // Non-copyable
  Map(const Map&) = delete;

  template <SomeFuture InputFuture>
  SomeFuture auto Pipe(InputFuture future) {
    return thunk::Map<InputFuture, F>{std::move(future), std::move(user)};
  }
};

}  // namespace pipe

/*
 * Functor
 * https://wiki.haskell.org/Typeclassopedia
 *
 * Future<T> -> (T -> U) -> Future<U>
 *
 */

template <typename F>
auto Map(F user) {
  return pipe::Map{std::move(user)};
}

}  // namespace exe::future

