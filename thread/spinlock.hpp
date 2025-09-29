#pragma once

#include <twist/trace/domain.hpp>
#include <twist/trace/scope.hpp>

#include <twist/ed/std/atomic.hpp>
#include <twist/ed/wait/spin.hpp>

namespace exe::thread {

class SpinLock {
 public:
  void Lock() {
    twist::trace::Scope scope{spinlock_, "lock"};

    while (!TryLock()) {
      twist::ed::SpinWait spin_wait;

      while (locked_.load(std::memory_order::relaxed)) {
        spin_wait();
      }
    }
  }

  bool TryLock() {
    twist::trace::Scope scope{spinlock_, "try_lock"};

    return !locked_.exchange(true, std::memory_order::acquire);
  }

  void Unlock() {
    twist::trace::Scope scope{spinlock_, "unlock"};

    locked_.store(false, std::memory_order::release);
  }

  // Lockable

  void lock() {  // NOLINT
    Lock();
  }

  bool try_lock() {  // NOLINT
    return TryLock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  twist::ed::std::atomic<bool> locked_{false};

  twist::trace::Domain spinlock_{"Spinlock"};
};

}  // namespace exe::thread

