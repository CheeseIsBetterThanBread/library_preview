#pragma once

#include <optional>

#include <exe/future/type/future.hpp>
#include <exe/future/syntax/pipe.hpp>
#include <exe/future/trait/value_of.hpp>

#include <exe/future/model/thunk.hpp>

#include <exe/runtime/view.hpp>
#include <exe/runtime/view/tasks.hpp>

#include <exe/runtime/task/task.hpp>
#include <exe/runtime/task/hint.hpp>

namespace exe::future {

namespace thunk {

template <Thunk Producer>
struct [[nodiscard]] Via {
  runtime::View runtime;
  Producer producer;

  explicit Via(runtime::View runtime, Producer producer)
      : runtime(runtime),
        producer(std::move(producer)) {
  }

  // Non-copyable
  Via(const Via&) = delete;
  Via& operator=(const Via&) = delete;

  Via(Via&&) = default;

  // Thunk
  using ValueType = trait::ValueOf<Producer>;

  template <Continuation<ValueType> Consumer>
  Computation auto Materialize(Consumer c) {
    return producer.Materialize(ViaTask<Consumer>{runtime, std::move(c)});
  }

  // Continuation
  template <Continuation<ValueType> Consumer>
  struct ViaTask : runtime::task::TaskBase {
    runtime::View runtime;
    Consumer consumer;

    std::optional<ValueType> value{std::nullopt};

    explicit ViaTask(runtime::View runtime, Consumer consumer)
        : runtime(runtime),
          consumer(std::move(consumer)) {
    }

    void Run() noexcept override {
      consumer.Continue(std::move(*value), {runtime});
    }

    void Continue(ValueType v, State) {
      value.emplace(std::move(v));
      runtime::Tasks(runtime).Submit(this,
                                     runtime::task::SchedulingHint::UpToYou);
    }
  };
};

}  // namespace thunk

namespace pipe {

struct [[nodiscard]] Via {
  runtime::View runtime;  // NOLINT

  explicit Via(runtime::View rt)
      : runtime(rt) {
  }

  // Non-copyable
  Via(const Via&) = delete;

  template <Thunk InputFuture>
  Future<trait::ValueOf<InputFuture>> auto Pipe(InputFuture future) {
    return thunk::Via<InputFuture>{runtime, std::move(future)};
  }
};

}  // namespace pipe

/*
 * Set runtime
 *
 * Future<T> -> Runtime -> Future<T>
 *
 */

inline auto Via(runtime::View runtime) {
  return pipe::Via{runtime};
}

}  // namespace exe::future

