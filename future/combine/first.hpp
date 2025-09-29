#pragma once

#include <optional>

#include <function2/function2.hpp>

#include <twist/assist/address.hpp>
#include <twist/ed/std/atomic.hpp>

#include <exe/future/type/future.hpp>

#include <exe/future/trait/value_of.hpp>
#include <exe/future/trait/materialize.hpp>

#include <exe/future/model/thunk.hpp>

#include <exe/runtime/view.hpp>

namespace exe::future {

namespace thunk {

template <Thunk LeftInput, Thunk RightInput>
struct [[nodiscard]] First {
  using Callback = fu2::unique_function<void()>;

  LeftInput left;
  RightInput right;

  explicit First(LeftInput left, RightInput right)
      : left(std::move(left)),
        right(std::move(right)) {
  }

  // Non-copyable
  First(const First&) = delete;
  First& operator=(const First&) = delete;

  First(First&&) = default;

  // Thunk
  using ValueType = trait::ValueOf<LeftInput>;

  template <Continuation<ValueType> Consumer>
  Computation auto Materialize(Consumer consumer) {
    return Race<Consumer>{std::move(left), std::move(right),
                          std::move(consumer)};
  }

  // Continuation
  template <Continuation<ValueType> Consumer>
  struct Bottleneck {
    twist::ed::std::atomic<bool>* first;
    Consumer* consumer_ptr;
    std::optional<Callback>* clean_up;

    Bottleneck(twist::ed::std::atomic<bool>* f, Consumer* c,
               std::optional<Callback>* cleaner)
        : first(f),
          consumer_ptr(c),
          clean_up(cleaner) {
    }

    void Continue(ValueType value, State state) {
      Consumer consumer = std::move(*consumer_ptr);
      if (first->exchange(false)) {
        consumer.Continue(std::move(value), state);
      } else {
        clean_up->value().operator()();
      }
    }
  };

  // Computation
  template <Continuation<ValueType> Consumer>
  struct Race {
    LeftInput left;
    RightInput right;
    Consumer* consumer;

    explicit Race(LeftInput left, RightInput right, Consumer consumer)
        : left(std::move(left)),
          right(std::move(right)),
          consumer(twist::assist::New<Consumer>(std::move(consumer))) {
    }

    void Start(runtime::View runtime) {
      using LeftComputation =
          trait::Materialize<LeftInput, Bottleneck<Consumer>>;
      using RightComputation =
          trait::Materialize<RightInput, Bottleneck<Consumer>>;

      auto first = twist::assist::New<twist::ed::std::atomic<bool>>(true);
      auto cleaner = twist::assist::New<std::optional<Callback>>(std::nullopt);

      Bottleneck<Consumer> left_cont{first, consumer, cleaner};
      Bottleneck<Consumer> right_cont{first, consumer, cleaner};

      auto left_comp = twist::assist::New<LeftComputation>(
          left.Materialize(std::move(left_cont)));
      auto right_comp = twist::assist::New<RightComputation>(
          right.Materialize(std::move(right_cont)));

      cleaner->emplace([atomic = first, consumer = this->consumer,
                        function = cleaner, left = left_comp,
                        right = right_comp] {
        delete atomic;
        delete consumer;
        delete left;
        delete right;
        delete function;
      });

      left_comp->Start(runtime);
      right_comp->Start(runtime);
    }
  };
};

}  // namespace thunk

template <Thunk LeftInput, Thunk RightInput>
Thunk auto First(LeftInput left, RightInput right) {
  return thunk::First<LeftInput, RightInput>(std::move(left), std::move(right));
}

}  // namespace exe::future

