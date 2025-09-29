#pragma once

#include <twist/assist/address.hpp>

#include <exe/runtime/view/tasks.hpp>
#include <exe/runtime/task/hint.hpp>

namespace exe::runtime {

namespace task::detail {

template <typename F>
struct BoxedTask final : TaskBase {
  explicit BoxedTask(F&& function)
      : user(std::move(function)) {
  }

  void Run() noexcept override {
    user();
    delete this;
  }

  F user;
};

}  // namespace task::detail

template <typename F>
void SubmitTask(View rt, F task) {
  auto box = twist::assist::New<task::detail::BoxedTask<F>>(std::move(task));
  Tasks(rt).Submit(box, task::SchedulingHint::UpToYou);
}

}  // namespace exe::runtime

