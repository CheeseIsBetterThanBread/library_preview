#pragma once

#include <twist/assist/assert.hpp>
#include <twist/trace/scope.hpp>

#include "record.hpp"
#include "reset_guard.hpp"
#include "managed.hpp"

namespace exe::runtime::hazard {

class PtrOwner {
  friend class Mutator;

 public:
  explicit PtrOwner(PtrRecord* hp)
      : hazard_ptr_(hp) {
  }

  // Non-copyable
  PtrOwner(const PtrOwner&) = delete;
  PtrOwner& operator=(const PtrOwner&) = delete;

  template <typename T>
  T* Protect(twist::ed::std::atomic<T*>& atomic_ptr,
             twist::trace::Scope = twist::trace::Scope(owner, "Protect")) {
    static_assert(IsManaged<T>);
    TWIST_ASSERT_M(IsValid(), "Not a valid owner");
    TWIST_ASSERT_M(hazard_ptr_->Get() == nullptr, "Overwriting hazard pointer");

    T* protected_ptr = nullptr;
    while (protected_ptr != atomic_ptr.load()) {
      do {
        protected_ptr = atomic_ptr.load();
      } while (protected_ptr != atomic_ptr.load());

      hazard_ptr_->Set(protected_ptr);
    }

    return protected_ptr;
  }

  template <typename T>
  void Set(T* obj, twist::trace::Scope = twist::trace::Scope(owner, "Set")) {
    static_assert(IsManaged<T>);
    TWIST_ASSERT_M(IsValid(), "Not a valid owner");
    TWIST_ASSERT_M(hazard_ptr_->Get() == nullptr, "Overwriting hazard pointer");

    if (obj == nullptr) {
      return;
    }

    hazard_ptr_->Set(obj);
  }

  void Reset(twist::trace::Scope = twist::trace::Scope(owner, "Reset")) {
    TWIST_ASSERT_M(IsValid(), "Not a valid owner");

    hazard_ptr_->Reset();
  }

  auto ScopedReset() {
    return PtrResetGuard{*this};
  }

  ~PtrOwner() {
    Reset();
    Release();
  }

 private:
  void Release() {
    if (!IsValid()) {
      return;
    }

    hazard_ptr_->Release();
    hazard_ptr_ = nullptr;
  }

  bool IsValid() const {
    return hazard_ptr_ != nullptr;
  }

 private:
  // Tracing
  static inline twist::trace::Domain owner{"HazardPtrOwner"};

 private:
  [[maybe_unused]] PtrRecord* hazard_ptr_;
};

}  // namespace exe::runtime::hazard

