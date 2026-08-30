#pragma once

#include "../future/future.hpp"

#include <exception>
#include <expected>
#include <memory>
#include <stdexcept>
#include <utility>

namespace sync {

template <typename T>
class Promise {
 private:
  std::shared_ptr<SharedState<T>> state_{std::make_shared<SharedState<T>>()};
  bool future_taken_{false};

  void Abandon();
  static std::exception_ptr NoState();

 public:
  Promise() = default;
  ~Promise();
  Promise(Promise&&) noexcept = default;
  Promise& operator=(Promise&& other) noexcept;
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  Future<T> GetFuture();

  template <typename... Args>
  std::expected<void, std::exception_ptr> SetValue(Args&&... args);
  std::expected<void, std::exception_ptr> SetException(std::exception_ptr error);
};

template <typename T>
std::exception_ptr Promise<T>::NoState() {
  return std::make_exception_ptr(std::logic_error("no state"));
}

template <typename T>
void Promise<T>::Abandon() {
  if (state_ && !state_->IsReady() && state_.use_count() > 1) {
    [[maybe_unused]] const auto discarded =
        state_->SetException(std::make_exception_ptr(std::logic_error("broken promise")));
  }
}

template <typename T>
Promise<T>::~Promise() {
  Abandon();
}

template <typename T>
Promise<T>& Promise<T>::operator=(Promise&& other) noexcept {
  if (this != &other) {
    Abandon();
    state_ = std::move(other.state_);
    future_taken_ = other.future_taken_;
    other.future_taken_ = false;
  }
  return *this;
}

template <typename T>
Future<T> Promise<T>::GetFuture() {
  if (!state_) {
    throw std::logic_error("no state");
  }
  if (future_taken_) {
    throw std::logic_error("future already retrieved");
  }
  future_taken_ = true;
  return Future<T>(state_);
}

template <typename T>
template <typename... Args>
std::expected<void, std::exception_ptr> Promise<T>::SetValue(Args&&... args) {
  if (!state_) {
    return std::unexpected(NoState());
  }
  return state_->SetValue(std::forward<Args>(args)...);
}

template <typename T>
std::expected<void, std::exception_ptr> Promise<T>::SetException(std::exception_ptr error) {
  if (!state_) {
    return std::unexpected(NoState());
  }
  return state_->SetException(std::move(error));
}

}  // namespace sync
