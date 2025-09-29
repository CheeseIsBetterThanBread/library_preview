#pragma once

#include <twist/assist/assert.hpp>

#include <twist/ed/std/atomic.hpp>

#include <vvv/list.hpp>

#include <exe/runtime/task/task.hpp>

#include "storage.hpp"

namespace exe::fiber::sync {

class LockFreeStack final : public LockFreeStorage {
  using Task = runtime::task::TaskBase;
  using Node = vvv::IntrusiveListNode<Task>;

  struct Dummy : Task {
    void Run() noexcept override {
    }
  };

 public:
  LockFreeStack() {
    top_.store(&dummy_);
  }

  bool IsClosed() const {
    return top_.load() == &dummy_;
  }

  void Open() {
    const Node* old = top_.exchange(nullptr);
    [[maybe_unused]] bool correct = old == nullptr || old == &dummy_;
    TWIST_ASSERT_M(correct, "Concurrent add call");
  }

  Node* Close() {
    return top_.exchange(&dummy_);
  }

  bool TryPush(Node* node) override {
    node->next = nullptr;

    while (!top_.compare_exchange_weak(node->next, node)) {
      if (node->next == &dummy_) {
        return false;
      }
    }

    return true;
  }

 private:
  twist::ed::std::atomic<Node*> top_{nullptr};
  Dummy dummy_;
};

}  // namespace exe::fiber::sync

