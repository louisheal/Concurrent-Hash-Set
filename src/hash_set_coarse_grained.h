#ifndef HASH_SET_COARSE_GRAINED_H
#define HASH_SET_COARSE_GRAINED_H

#include <cassert>
#include <functional>
#include <mutex>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template <typename T>
class HashSetCoarseGrained : public HashSetBase<T> {
 public:
  explicit HashSetCoarseGrained(size_t initial_capacity) : capacity_(initial_capacity) {
    table_.resize(capacity_);
    size_ = 0;
  }

  bool Add(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;

    mutex_.lock();
    auto result = table_[bucketNum].insert(elem).second;
    size_ += result ? 1 : 0;
    mutex_.unlock();

    return result;
  }

  bool Remove(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;

    mutex_.lock();
    bool result = table_[bucketNum].erase(elem);
    size_ -= result ? 1 : 0;
    mutex_.unlock();

    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;

    mutex_.lock();
    size_t result = table_[bucketNum].count(elem);
    mutex_.unlock();

    return result == 1;
  }

  [[nodiscard]] size_t Size() const final {
    return size_;
  }

  private:
    size_t capacity_;
    size_t size_;
    std::vector<std::set<T>> table_;
    std::mutex mutex_;
};

#endif  // HASH_SET_COARSE_GRAINED_H
