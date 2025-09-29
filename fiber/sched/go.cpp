#include <twist/assist/assert.hpp>
#include <twist/assist/address.hpp>

#include <exe/runtime/view/tasks.hpp>

#include <exe/fiber/core/fiber.hpp>

#include "go.hpp"

namespace exe::fiber {

void Go(runtime::View runtime, Body body) {
  Stack* stack = runtime::StackAllocator(runtime).Allocate();
  TWIST_ASSERT_M(stack != nullptr, "Allocator returned invalid ptr");

  auto newbie = twist::assist::New<Fiber>(runtime, std::move(body), stack);
  runtime::Tasks(runtime).Submit(newbie);
}

void Go(Body body) {
  runtime::View current_runtime = Fiber::Self().GetRuntime();
  Go(current_runtime, std::move(body));
}

}  // namespace exe::fiber

