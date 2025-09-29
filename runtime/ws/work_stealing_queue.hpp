#pragma once

#include <array>
#include <span>

#include <twist/ed/std/atomic.hpp>

#include <twist/assist/preempt.hpp>

#include <vvv/list.hpp>

#include <exe/runtime/task/task.hpp>

namespace exe::runtime::multi_thread::v2 {

template <size_t Capacity>
class WorkStealingTaskQueue {
  using Task = task::TaskBase;

  class Slot {
   public:
    Task* Get() const {
      twist::assist::NoPreemptionGuard guard;
      return task_.load(std::memory_order::relaxed);
    }

    void Set(Task* task) {
      twist::assist::NoPreemptionGuard guard;
      task_.store(task, std::memory_order::relaxed);
    }

   private:
    twist::ed::std::atomic<Task*> task_{nullptr};
  };

 public:
  bool TryPush(Task* task) {
    const size_t current_head = head_.load(std::memory_order::relaxed);
    const size_t current_tail = tail_.load(std::memory_order::relaxed);

    if (IsFull(current_head, current_tail)) {
      return false;
    }

    buffer_[current_tail % Capacity].Set(task);
    tail_.store(current_tail + 1, std::memory_order::release);
    return true;
  }

  size_t PushMany(vvv::IntrusiveList<Task>& tasks) {
    const size_t current_head = head_.load(std::memory_order::relaxed);
    const size_t current_tail = tail_.load(std::memory_order::relaxed);

    size_t pushed = 0;
    while (tasks.NonEmpty() && !IsFull(current_head, current_tail + pushed)) {
      buffer_[(current_tail + pushed) % Capacity].Set(tasks.PopFrontNonEmpty());
      ++pushed;
    }

    if (pushed != 0) {
      tail_.store(current_tail + pushed, std::memory_order::release);
    }

    return pushed;
  }

  Task* TryPop() {
    while (true) {
      size_t current_head = head_.load(std::memory_order::relaxed);
      const size_t current_tail = tail_.load(std::memory_order::relaxed);

      if (IsEmpty(current_head, current_tail)) {
        return nullptr;
      }

      Task* task = buffer_[current_head % Capacity].Get();
      if (head_.compare_exchange_weak(current_head, current_head + 1,
                                      std::memory_order::relaxed)) {
        return task;
      }
    }
  }

  size_t Grab(std::span<Task*> out_buffer) {
    const size_t buffer_limit = out_buffer.size();

    while (true) {
      size_t current_head = head_.load(std::memory_order::relaxed);
      const size_t current_tail = tail_.load(std::memory_order::acquire);
      if (IsEmpty(current_head, current_tail)) {
        return 0;
      }

      const size_t can_steal = 1 + (current_tail - current_head) / 2;
      const size_t limit = std::min(can_steal, buffer_limit);

      for (size_t i = 0; i < limit; ++i) {
        out_buffer[i] = buffer_[(current_head + i) % Capacity].Get();
      }
      if (head_.compare_exchange_weak(current_head, current_head + limit,
                                      std::memory_order::relaxed)) {
        return limit;
      }
    }
  }

  bool CurrentlyHasItems() const {
    const size_t current_head = head_.load(std::memory_order::relaxed);
    const size_t current_tail = tail_.load(std::memory_order::relaxed);

    return !IsEmpty(current_head, current_tail);
  }

 private:
  static bool IsEmpty(const size_t head, const size_t tail) {
    return tail == head;
  }

  static bool IsFull(const size_t head, const size_t tail) {
    return tail - head == Capacity;
  }

  std::array<Slot, Capacity> buffer_;

  twist::ed::std::atomic<size_t> head_{0};
  twist::ed::std::atomic<size_t> tail_{0};
};

}  // namespace exe::runtime::multi_thread::v2

