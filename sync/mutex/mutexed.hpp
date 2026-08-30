#pragma once

#include "mutex.hpp"
#include "raii/unique_lock.hpp"

#include <utility>

namespace sync {

template <typename T>
class Mutexed {
 public:
  class OwnerRef {
   public:
    OwnerRef(FutexMutex& mutex, T& object) : lock_(mutex), object_(&object) {}

    OwnerRef(const OwnerRef&) = delete;
    OwnerRef& operator=(const OwnerRef&) = delete;
    OwnerRef(OwnerRef&&) = default;
    OwnerRef& operator=(OwnerRef&&) = default;

    T* operator->() {
      return object_;
    }
    const T* operator->() const {
      return object_;
    }

    T& operator*() {
      return *object_;
    }
    const T& operator*() const {
      return *object_;
    }

   private:
    UniqueLock lock_;
    T* object_;
  };

  template <class... Args>
  explicit Mutexed(Args&&... args) : t_(std::forward<Args>(args)...) {}

  OwnerRef Acquire() {
    return OwnerRef(mutex_, t_);
  }

 private:
  FutexMutex mutex_;
  T t_;
};

}  // namespace sync
