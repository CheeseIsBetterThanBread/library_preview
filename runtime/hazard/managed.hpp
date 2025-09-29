#pragma once

#include <concepts>

#include <vvv/list.hpp>

namespace exe::runtime::hazard {

struct IManaged : vvv::IntrusiveListNode<IManaged> {
  virtual void Delete() = 0;
  virtual ~IManaged() = default;
};

template <typename T>
struct Managed : IManaged {
  void Delete() override {
    auto* actual = static_cast<T*>(this);
    delete actual;
  }
};

template <typename T>
concept IsManaged = std::derived_from<T, Managed<T>>;

}  // namespace exe::runtime::hazard

