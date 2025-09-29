#pragma once

#include <optional>
#include <vector>

#include "detail/state.hpp"
#include "fwd.hpp"

namespace exe::runtime::multi_thread::v2 {

class Coordinator {
 public:
  explicit Coordinator(ThreadPool&);

  void NotifyOnSubmit();

  // Returns true if worker needs to do a final check
  bool TransitionToParked(size_t worker_id, bool is_searching);

  // Returns true if transition happened
  bool TransitionToSearching();

  // Returns true if worker was last searching
  bool TransitionFromSearching();

 private:
  // If there are no searching workers, returns the index of parked one
  std::optional<size_t> WorkerToNotify();

 private:
  ThreadPool& host_;

  // Indices of parked workers
  std::vector<size_t> parked_;

  // Try to limit park / unpark calls
  detail::State state_;
};

}  // namespace exe::runtime::multi_thread::v2

