#pragma once

#include <array>
#include <variant>

#include <exe/thread/spinlock.hpp>

#include <exe/util/defer.hpp>
#include <exe/util/random.hpp>

#include <exe/fiber/chan/detail/select/clause.hpp>
#include <exe/fiber/chan/detail/select/select_info.hpp>
#include <exe/fiber/chan/detail/select/variant.hpp>

namespace exe::fiber {

namespace detail {

// all locks must not be held before this call
// also locks must be sorted
template <size_t N>
void Lock(const std::array<thread::SpinLock*, N>& locks) {
  for (auto* lock : locks) {
    if (lock == nullptr) {
      continue;
    }

    lock->lock();
  }
}

// all locks must be held before this call
template <size_t N>
void Unlock(const std::array<thread::SpinLock*, N>& locks) {
  for (auto* lock : locks) {
    if (lock == nullptr) {
      continue;
    }

    lock->unlock();
  }
}

template <typename... Ts>
struct TypePack {
  template <template <typename...> class Func, typename... Args>
  static auto Apply(Args&&... args) {
    return Func<Ts...>::Construct(std::forward<Args>(args)...);
  }
};

}  // namespace detail

template <typename... ClauseTypes>
  requires(std::derived_from<ClauseTypes, detail::IClause> && ...)
auto Select(ClauseTypes... clause_objects)
    -> std::variant<typename ClauseTypes::ReturnType...> {
  constexpr size_t kSize = sizeof...(clause_objects);
  static_assert(kSize > 1);
  detail::SelectInfo info;

  const std::array<detail::IClause*, kSize> clauses = {(&clause_objects)...};
  for (size_t i = 0; i < kSize; ++i) {
    auto* clause = clauses[i];
    clause->Setup(&info, i);
  }

  std::array<thread::SpinLock*, kSize> locks{};
  for (size_t i = 0; i < kSize; ++i) {
    locks[i] = clauses[i]->GetLockPtr();
  }
  std::ranges::sort(locks);

  detail::Lock(locks);
  Defer defer([&] {
    detail::Unlock(locks);
  });

  const size_t start = RandomGeneration() % kSize;
  using ValueTypes = detail::TypePack<typename ClauseTypes::ReturnType...>;

  // Pass 1: try fast path
  for (size_t index = 0; index < kSize; ++index) {
    const size_t actual = (start + index) % kSize;
    if (clauses[actual]->TryAct()) {
      return ValueTypes::template Apply<detail::VariantConstructor>(
          actual, clauses[actual]->GetValue());
    }
  }

  // Pass 2: enqueue on all channels
  for (auto* clause : clauses) {
    clause->Enqueue();
  }

  detail::Unlock(locks);
  info.signal.Wait();
  detail::Lock(locks);

  // Pass 3: dequeue from all channels and return value
  for (auto* clause : clauses) {
    clause->Dequeue();
  }

  const size_t successful = info.flag.load();
  return ValueTypes::template Apply<detail::VariantConstructor>(
      successful, clauses[successful]->GetValue());
}

}  // namespace exe::fiber

