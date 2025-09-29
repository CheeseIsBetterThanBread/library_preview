#pragma once

#include <exe/runtime/view.hpp>
#include <exe/runtime/task/task.hpp>

#include <exe/fiber/sched/awaiter.hpp>

#include "body.hpp"
#include "coroutine.hpp"
#include "stack.hpp"

namespace exe::fiber {

// Fiber = Stackful coroutine x Runtime

// NOLINTNEXTLINE
class Fiber final : public runtime::task::TaskBase {
 public:
  Fiber(runtime::View, Body, Stack*);

  ~Fiber();

  runtime::View GetRuntime() const;

  void Syscall(IAwaiter* handler);

  static Fiber& Self();

  void Run() noexcept override;

 private:
  runtime::View runtime_;
  Coroutine coroutine_;
  IAwaiter* handler_{nullptr};
};

}  // namespace exe::fiber

