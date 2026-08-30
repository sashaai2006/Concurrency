#pragma once

#include "block_queue.hpp"

#include <cstddef>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace sync {

template <typename Task>
class ThreadPool {
 private:
  BlockQueue<Task> tasks_;
  std::vector<std::thread> workers_;
  void Work();

 public:
  ThreadPool(size_t nof_threads);
  ~ThreadPool();
  template <typename U>
  void Add(U&& task);
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
template <typename U>
void ThreadPool<Task>::Add(U&& task) {
  tasks_.Push(std::forward<U>(task));
}

}  // namespace sync
