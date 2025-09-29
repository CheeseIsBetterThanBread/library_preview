#pragma once

#include <twist/ed/std/atomic.hpp>
#include <twist/assist/atomic.hpp>

#include <exe/fiber/sched/suspend.hpp>

#include "stack.hpp"
#include "storage_awaiter.hpp"
#include "wake.hpp"

namespace exe::fiber {

class WaitGroup {
 public:
  void Add(const uint64_t delta) {
    const uint64_t state = IncreaseBalance(delta);
    const uint64_t balance = GetBalance(state);

    // first add
    if (balance == delta) {
      stack_.Open();
    }
  }

  void Done() {
    const uint64_t state = DecreaseBalance();

    const uint64_t balance = GetBalance(state);
    const uint64_t waiters = GetWaiters(state);

    if (balance == 0 && waiters > 0) {
      vvv::IntrusiveListNode<runtime::task::TaskBase>* head = stack_.Close();
      sync::WakeUp(head);
    }
  }

  void Wait() {
    uint64_t state;

    do {
      state = state_.load();

      if (const uint64_t balance = GetBalance(state); balance == 0) {
        return;
      }
    } while (!state_.compare_exchange_strong(state, state + 1));

    if (!stack_.IsClosed()) {
      sync::detail::StorageAwaiter awaiter{stack_};
      Suspend(&awaiter);
    }

    state_.store(0);
  }

 private:
  static constexpr uint64_t kBalanceShift = 32;
  static constexpr uint64_t kSingleBalance = 1uz << kBalanceShift;
  static constexpr uint64_t kWaitersMask = kSingleBalance - 1;

  uint64_t IncreaseBalance(const uint64_t delta) {
    const uint64_t state = state_.fetch_add(kSingleBalance * delta);
    return state + kSingleBalance * delta;
  }

  uint64_t DecreaseBalance() {
    const uint64_t state = state_.fetch_sub(kSingleBalance);
    return state - kSingleBalance;
  }

  static uint64_t GetBalance(const uint64_t state) {
    return state >> kBalanceShift;
  }

  static uint64_t GetWaiters(const uint64_t state) {
    return state & kWaitersMask;
  }

  sync::LockFreeStack stack_;
  twist::ed::std::atomic<twist::assist::Packed<uint64_t, 32, 32>> state_{0};
};

}  // namespace exe::fiber

