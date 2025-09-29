#include <exe/fiber/core/handle.hpp>

#include <exe/runtime/view/tasks.hpp>
#include <exe/runtime/task/hint.hpp>

#include "awaiter.hpp"
#include "suspend.hpp"
#include "yield.hpp"

namespace exe::fiber {

namespace detail {

struct YieldAwaiter final : IAwaiter,
                            runtime::task::TaskBase {
  void Run() noexcept override {
    handle.Resume();
  }

  FiberHandle AwaitSuspend(FiberHandle self) override {
    handle = self;
    auto runtime = self.GetRuntime();
    runtime::Tasks(runtime).Submit(this, runtime::task::SchedulingHint::Yield);
    return FiberHandle::Invalid();
  }

  FiberHandle handle;
};

}  // namespace detail

void Yield() {
  detail::YieldAwaiter yield;
  Suspend(&yield);
}

}  // namespace exe::fiber

