#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace sync {

template <typename T>
class BlockQueue {
 private:
  std::queue<T> queue_;
  std::mutex mtx_;
  std::condition_variable cv_;
  bool open_;

 public:
  BlockQueue();
  BlockQueue(const BlockQueue&) = delete;
  BlockQueue& operator=(const BlockQueue&) = delete;
  BlockQueue(BlockQueue&&) = delete;
  BlockQueue& operator=(BlockQueue&&) = delete;
  ~BlockQueue();
  template <typename U>
  void Push(U&& t);
  std::optional<T> Get();
  bool Empty();
  size_t Size();
  void Close();
};

template <typename T>
BlockQueue<T>::BlockQueue() : open_{true} {}

template <typename T>
BlockQueue<T>::~BlockQueue() {
  Close();
}

template <typename T>
void BlockQueue<T>::Close() {
  std::lock_guard lock(mtx_);
  open_ = false;
  cv_.notify_all();
}

template <typename T>
template <typename U>
void BlockQueue<T>::Push(U&& t) {
  {
    std::lock_guard lock(mtx_);
    if (!open_) {
      return;
    }
    queue_.push(std::forward<U>(t));
  }
  cv_.notify_one();
}

template <typename T>
std::optional<T> BlockQueue<T>::Get() {
  std::unique_lock lock(mtx_);
  cv_.wait(lock, [&]() -> bool { return !(open_ && queue_.empty()); });
  if (queue_.empty()) {
    return std::nullopt;
  }
  T t = std::move(queue_.front());
  queue_.pop();
  return t;
}

template <typename T>
bool BlockQueue<T>::Empty() {
  std::lock_guard lock(mtx_);
  return queue_.empty();
}

template <typename T>
size_t BlockQueue<T>::Size() {
  std::lock_guard lock(mtx_);
  return queue_.size();
}

}  // namespace sync
