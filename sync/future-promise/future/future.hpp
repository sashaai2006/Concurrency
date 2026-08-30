#pragma once

#include "../shared-state/shared_state.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace sync {

template <typename T>
class Promise;

template <typename T>
class Future {
 private:
  friend Promise<T>;
  std::shared_ptr<SharedState<T>> state_;
  explicit Future(std::shared_ptr<SharedState<T>> state);

 public:
  Future() = default;
  ~Future() = default;
  Future(Future&&) noexcept = default;
  Future& operator=(Future&&) noexcept = default;
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  bool IsValid() const;
  bool IsReady();
  void Wait();
  T Get();
};

template <typename T>
Future<T>::Future(std::shared_ptr<SharedState<T>> state) : state_(std::move(state)) {}

template <typename T>
bool Future<T>::IsValid() const {
  return state_ != nullptr;
}

template <typename T>
bool Future<T>::IsReady() {
  return IsValid() && state_->IsReady();
}

template <typename T>
void Future<T>::Wait() {
  if (!IsValid()) {
    throw std::logic_error("no state");
  }
  state_->Wait();
}

template <typename T>
T Future<T>::Get() {
  if (!IsValid()) {
    throw std::logic_error("no state");
  }
  auto state = std::move(state_);
  return state->Take();
}

}  // namespace sync
