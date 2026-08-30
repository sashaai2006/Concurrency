#pragma once

#include "../future-promise/promise/promise.hpp"
#include "block_queue.hpp"

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace sync {

namespace detail {

class PoolWork {
 private:
  struct Base {
    virtual ~Base() = default;
    virtual void Run() = 0;
  };

  template <typename F>
  struct Impl : Base {
    F fn_;
    explicit Impl(F fn) : fn_(std::move(fn)) {}
    void Run() override {
      fn_();
    }
  };

  std::unique_ptr<Base> impl_;

 public:
  PoolWork() = default;
  PoolWork(PoolWork&&) noexcept = default;
  PoolWork& operator=(PoolWork&&) noexcept = default;
  PoolWork(const PoolWork&) = delete;
  PoolWork& operator=(const PoolWork&) = delete;

  template <typename F>
    requires(!std::is_same_v<std::decay_t<F>, PoolWork>)
  explicit PoolWork(F&& fn) : impl_(std::make_unique<Impl<std::decay_t<F>>>(std::forward<F>(fn))) {}

  void operator()() {
    impl_->Run();
  }
};

}  // namespace detail

template <typename Task>
class ThreadPool {
 private:
  BlockQueue<detail::PoolWork> tasks_;
  std::vector<std::thread> workers_;
  void Work();

 public:
  ThreadPool(size_t nof_threads);
  ~ThreadPool();

  template <typename F>
  auto Submit(F&& fn) -> Future<std::invoke_result_t<std::decay_t<F>>>;
};

template <typename Task>
void ThreadPool<Task>::Work() {
  while (true) {
    auto task = tasks_.Get();
    if (!task.has_value()) {
      break;
    }
    try {
      (task.value())();
    } catch (...) {
    }
  }
}

template <typename Task>
ThreadPool<Task>::ThreadPool(size_t nof_threads) {
  if (nof_threads == 0) {
    throw std::invalid_argument("ThreadPool: thread_count must be > 0");
  }
  workers_.reserve(nof_threads);
  try {
    for (size_t i = 0; i < nof_threads; ++i) {
      workers_.emplace_back([this]() { Work(); });
    }
  } catch (...) {
    tasks_.Close();
    for (auto& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
    throw;
  }
}

template <typename Task>
ThreadPool<Task>::~ThreadPool() {
  tasks_.Close();
  for (auto& worker : workers_) {
    worker.join();
  }
}

template <typename Task>
template <typename F>
auto ThreadPool<Task>::Submit(F&& fn) -> Future<std::invoke_result_t<std::decay_t<F>>> {
  using R = std::invoke_result_t<std::decay_t<F>>;
  Promise<R> promise;
  auto future = promise.GetFuture();
  tasks_.Push(detail::PoolWork(
      [fn = std::decay_t<F>(std::forward<F>(fn)), promise = std::move(promise)]() mutable {
        try {
          if constexpr (std::is_void_v<R>) {
            std::invoke(fn);
            [[maybe_unused]] const auto discarded = promise.SetValue();
          } else {
            [[maybe_unused]] const auto discarded = promise.SetValue(std::invoke(fn));
          }
        } catch (...) {
          [[maybe_unused]] const auto discarded = promise.SetException(std::current_exception());
        }
      }));
  return future;
}

}  // namespace sync
