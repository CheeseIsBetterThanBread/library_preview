#include <twist/assist/assert.hpp>
#include <twist/ed/static/thread_local/ptr.hpp>

#include <exe/runtime/view/tasks.hpp>

#include "fiber.hpp"
#include "handle.hpp"

namespace exe::fiber {

TWISTED_STATIC_THREAD_LOCAL_PTR(Fiber, current_fiber);

Fiber::Fiber(runtime::View view, Body body, Stack* stack)
    : runtime_(view),
      coroutine_(std::move(body), stack) {
}

Fiber::~Fiber() {
  runtime::StackAllocator(runtime_).Release(coroutine_.stack_);
}

void Fiber::Run() noexcept {
  current_fiber = this;
  coroutine_.Resume();
  current_fiber = nullptr;

  if (coroutine_.IsDone()) {
    delete this;
    return;
  }

  FiberHandle result = handler_->AwaitSuspend(FiberHandle(this));
  if (result.IsValid()) {
    result.Resume();
  }
}

Fiber& Fiber::Self() {
  TWIST_ASSERT_M(current_fiber != nullptr, "Not in fiber");
  return *current_fiber;
}

runtime::View Fiber::GetRuntime() const {
  return runtime_;
}

void Fiber::Syscall(IAwaiter* handler) {
  handler_ = handler;
  coroutine_.Suspend();
}

}  // namespace exe::fiber

