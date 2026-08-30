#pragma once

#include <condition_variable>
#include <exception>
#include <expected>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace sync {

template <typename T>
class SharedState {
 private:
  std::exception_ptr error_;
  std::mutex mutex_;
  std::condition_variable ready_;
  std::optional<T> result_;

  bool HasResult() const;
  static std::exception_ptr AlreadySatisfied();

 public:
  SharedState() = default;
  ~SharedState() = default;
  SharedState(const SharedState&) = delete;
  SharedState& operator=(const SharedState&) = delete;
  SharedState(SharedState&&) = delete;
  SharedState& operator=(SharedState&&) = delete;

  template <typename... Args>
  std::expected<void, std::exception_ptr> SetValue(Args&&... args);
  std::expected<void, std::exception_ptr> SetException(std::exception_ptr error);

  bool IsReady();
  void Wait();
  T Take();
};

template <typename T>
bool SharedState<T>::HasResult() const {
  return result_.has_value() || static_cast<bool>(error_);
}

template <typename T>
std::exception_ptr SharedState<T>::AlreadySatisfied() {
  return std::make_exception_ptr(std::logic_error("already satisfied"));
}

template <typename T>
bool SharedState<T>::IsReady() {
  std::lock_guard guard(mutex_);
  return HasResult();
}

template <typename T>
template <typename... Args>
std::expected<void, std::exception_ptr> SharedState<T>::SetValue(Args&&... args) {
  std::lock_guard guard(mutex_);
  if (HasResult()) {
    return std::unexpected(AlreadySatisfied());
  }
  result_.emplace(std::forward<Args>(args)...);
  ready_.notify_all();
  return {};
}

template <typename T>
std::expected<void, std::exception_ptr> SharedState<T>::SetException(std::exception_ptr error) {
  if (!error) {
    return std::unexpected(std::make_exception_ptr(std::logic_error("null exception")));
  }
  std::lock_guard guard(mutex_);
  if (HasResult()) {
    return std::unexpected(AlreadySatisfied());
  }
  error_ = std::move(error);
  ready_.notify_all();
  return {};
}

template <typename T>
void SharedState<T>::Wait() {
  std::unique_lock lock(mutex_);
  ready_.wait(lock, [this] { return HasResult(); });
}

template <typename T>
T SharedState<T>::Take() {
  std::unique_lock lock(mutex_);
  ready_.wait(lock, [this] { return HasResult(); });
  if (error_) {
    std::rethrow_exception(error_);
  }
  return std::move(*result_);
}

template <>
class SharedState<void> {
 private:
  std::exception_ptr error_;
  std::mutex mutex_;
  std::condition_variable ready_;
  bool done_{false};

  bool HasResult() const;
  static std::exception_ptr AlreadySatisfied();

 public:
  SharedState() = default;
  ~SharedState() = default;
  SharedState(const SharedState&) = delete;
  SharedState& operator=(const SharedState&) = delete;
  SharedState(SharedState&&) = delete;
  SharedState& operator=(SharedState&&) = delete;

  std::expected<void, std::exception_ptr> SetValue();
  std::expected<void, std::exception_ptr> SetException(std::exception_ptr error);

  bool IsReady();
  void Wait();
  void Take();
};

inline bool SharedState<void>::HasResult() const {
  return done_ || static_cast<bool>(error_);
}

inline std::exception_ptr SharedState<void>::AlreadySatisfied() {
  return std::make_exception_ptr(std::logic_error("already satisfied"));
}

inline bool SharedState<void>::IsReady() {
  std::lock_guard guard(mutex_);
  return HasResult();
}

inline std::expected<void, std::exception_ptr> SharedState<void>::SetValue() {
  std::lock_guard guard(mutex_);
  if (HasResult()) {
    return std::unexpected(AlreadySatisfied());
  }
  done_ = true;
  ready_.notify_all();
  return {};
}

inline std::expected<void, std::exception_ptr> SharedState<void>::SetException(
    std::exception_ptr error) {
  if (!error) {
    return std::unexpected(std::make_exception_ptr(std::logic_error("null exception")));
  }
  std::lock_guard guard(mutex_);
  if (HasResult()) {
    return std::unexpected(AlreadySatisfied());
  }
  error_ = std::move(error);
  ready_.notify_all();
  return {};
}

inline void SharedState<void>::Wait() {
  std::unique_lock lock(mutex_);
  ready_.wait(lock, [this] { return HasResult(); });
}

inline void SharedState<void>::Take() {
  std::unique_lock lock(mutex_);
  ready_.wait(lock, [this] { return HasResult(); });
  if (error_) {
    std::rethrow_exception(error_);
  }
}

}  // namespace sync
