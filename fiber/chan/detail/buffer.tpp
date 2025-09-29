#pragma once

#include <twist/assist/assert.hpp>

#include "buffer.hpp"

namespace exe::fiber::detail {

template <typename T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : queue_(capacity),
      capacity_(capacity) {
}

template <typename T>
size_t RingBuffer<T>::Next(size_t index) const {
  ++index;
  if (index >= capacity_) {
    return index - capacity_;
  }
  return index;
}

template <typename T>
bool RingBuffer<T>::Empty() const {
  return size_ == 0;
}

template <typename T>
size_t RingBuffer<T>::Size() const {
  return size_;
}

template <typename T>
void RingBuffer<T>::Push(T value) {
  TWIST_ASSERT_M(size_ < capacity_, "Buffer is full");
  queue_[tail_].emplace(std::move(value));

  ++size_;
  tail_ = Next(tail_);
}

template <typename T>
T RingBuffer<T>::Pop() {
  TWIST_ASSERT_M(size_ != 0, "Buffer is empty");
  T value = std::move(queue_[head_].value());

  --size_;
  head_ = Next(head_);

  return value;
}

}  // namespace exe::fiber::detail

