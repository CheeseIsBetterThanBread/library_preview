#pragma once

#include <optional>
#include <random>
#include <span>

#include <twist/ed/std/thread.hpp>
#include <twist/build.hpp>

#include <vvv/list.hpp>

#include <exe/runtime/task/task.hpp>
#include <exe/runtime/task/hint.hpp>

#include <exe/runtime/hazard/managed.hpp>
#include <exe/runtime/hazard/record.hpp>

#include "fwd.hpp"
#include "work_stealing_queue.hpp"

#include "detail/metrics.hpp"
#include "detail/parking_lot.hpp"

namespace exe::runtime::multi_thread::v2 {

class Worker {
  static constexpr size_t kLocalQueueCapacity =
      twist::build::kTwisted ? 7 : 256;

 public:
  Worker(ThreadPool& host, size_t index);

  void Start();
  void Join();

  // Single producer
  // Tasks are linked using field next, hint != Yield
  void Push(task::TaskBase*, task::SchedulingHint);

  // Steal from this worker
  size_t StealTasks(std::span<task::TaskBase*> out_buffer);

  // Wake parked worker
  void Wake();

  static Worker* Current();

  ThreadPool* Host() const {
    return &host_;
  }

  void Retire(hazard::IManaged*);

  vvv::IntrusiveList<hazard::IManaged> GetRetired();

  hazard::PtrRecord* GetHazardPtr();

  hazard::PtrRecord* GetHazardHead() const;

 private:
  void FaultInjection(std::span<task::TaskBase*> buffer);

  // Use in Push
  void PushToLifoSlot(task::TaskBase* task);
  void PushToLocalQueue(task::TaskBase* task);
  void OffloadTasksToGlobalQueue(vvv::IntrusiveList<task::TaskBase> overflow);

  // Use in TryPickTask
  task::TaskBase* TryPickTaskFromLifoSlot();
  task::TaskBase* TryPickTaskFromLocalQueue();
  task::TaskBase* TryStealTasks();
  task::TaskBase* TryPickTaskFromGlobalQueue() const;
  task::TaskBase* TryGrabTasksFromGlobalQueue();

  void Park();

  void Maintenance();

  task::TaskBase* PickTask();  // Or park thread

  // Run Loop
  void Work();

  // Returns true on success
  bool TransitionToSearching();

  void TransitionFromSearching();

  // Returns true on success, in case of false there is some work to do
  bool TransitionToParked();

  void NotifyIfWorkPending() const;

  // Convert tasks linked by next into intrusive list
  static vvv::IntrusiveList<task::TaskBase> Convert(task::TaskBase* task);

 private:
  ThreadPool& host_;
  const size_t index_;

  // Scheduling iteration
  size_t iter_{0};

  // Worker thread
  std::optional<twist::ed::std::thread> thread_{std::nullopt};

  // Local queue
  WorkStealingTaskQueue<kLocalQueueCapacity> local_queue_{};

  // LIFO slot
  task::TaskBase* lifo_slot_{nullptr};
  size_t lifo_streak_{0};

  // Deterministic pseudo-randomness for work stealing
  std::mt19937_64 twister_;

  // Worker state
  bool is_searching_{false};

  // Collecting metrics
  detail::Metrics metrics_{};

  // Parking lot
  detail::ParkingLot parking_lot_{};

  // Garbage collection
  twist::ed::std::atomic<hazard::PtrRecord*> hazard_top_{nullptr};
  vvv::IntrusiveList<hazard::IManaged> retired_;
  size_t retired_size_{0};
};

}  // namespace exe::runtime::multi_thread::v2

