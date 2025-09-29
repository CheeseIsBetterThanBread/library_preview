#pragma once

#include <optional>
#include <utility>  // std::move

#include <exe/future/trait/value_of.hpp>

#include <exe/runtime/run_loop.hpp>

#include <exe/future/model/comp.hpp>
#include <exe/future/model/state.hpp>

namespace exe::future {

/*
 * Unwraps Future synchronously (blocking current thread)
 *
 * Usage:
 *
 * auto f = future::Spawn(runtime, [] { return 7; }));
 * auto v = future::Get(std::move(f));
 *
 */

namespace detail {

template <typename V>
struct Getter {
  runtime::RunLoop& run_loop;
  std::optional<V>& result;

  explicit Getter(runtime::RunLoop& run_loop, std::optional<V>& result)
      : run_loop(run_loop),
        result(result) {
  }

  // Demand
  void Continue(V value, State) {
    result.emplace(std::move(value));
    run_loop.Stop();
  }
};

}  // namespace detail

template <SomeFuture Future>
trait::ValueOf<Future> Get(Future future) {
  using Value = trait::ValueOf<Future>;

  runtime::RunLoop run_loop;
  std::optional<Value> result{std::nullopt};
  auto getter = detail::Getter<Value>{run_loop, result};

  Computation auto computation = future.Materialize(std::move(getter));
  computation.Start(run_loop);

  run_loop.Run();

  return std::move(*result);
}

}  // namespace exe::future

