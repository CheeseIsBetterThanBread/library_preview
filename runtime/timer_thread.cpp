#include <optional>

#include <vvv/list.hpp>

#include <twist/assist/assert.hpp>
#include <twist/ed/wait/futex.hpp>

#include "timer_thread.hpp"

namespace exe::runtime::multi_thread {

TimerThread::TimerThread(task::IScheduler* actual)
    : thread_(std::nullopt),
      scheduler_(*actual) {
}

TimerThread::~TimerThread() {
  TWIST_ASSERT_M(!running_.load(), "TimerThread is still running");
}

void TimerThread::Set(const timer::Duration delay, timer::TimerBase* handler) {
  const auto offset =
      std::chrono::duration_cast<timer::Duration>(Clock::now() - start_);
  const auto delta = offset + delay;
  handler->Set(delta);
  buffer_.Push(handler);
}

void TimerThread::Start() {
  running_.store(true);
  start_ = Clock::now();
  thread_ = twist::ed::std::thread(&TimerThread::Run, this);
}

void TimerThread::Stop() {
  running_.store(false);
  thread_.value().join();
}

void TimerThread::Relocate() {
  vvv::IntrusiveList<task::TaskBase> arrived = buffer_.PopAll();
  if (arrived.IsEmpty()) {
    return;
  }

  vvv::IntrusiveListNode<task::TaskBase>* current = arrived.FrontNonEmpty();
  const vvv::IntrusiveListNode<task::TaskBase>* sentinel = current->prev;

  while (current != sentinel) {
    vvv::IntrusiveListNode<task::TaskBase>* old = current;
    current = current->next;

    old->Unlink();
    auto* actual = dynamic_cast<timer::TimerBase*>(old->AsItem());
    queue_.Push(actual);
  }
}

void TimerThread::Run() {
  using namespace std::chrono_literals;  // NOLINT

  while (running_.load()) {
    Relocate();

    const auto offset =
        std::chrono::duration_cast<timer::Duration>(Clock::now() - start_);
    vvv::IntrusiveList<task::TaskBase> expired = queue_.PopExpired(offset);
    scheduler_.SubmitMany(std::move(expired));

    twist::ed::std::this_thread::sleep_for(50us);
  }
}

}  // namespace exe::runtime::multi_thread

