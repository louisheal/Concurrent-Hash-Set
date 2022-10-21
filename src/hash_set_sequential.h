#ifndef HASH_SET_SEQUENTIAL_H
#define HASH_SET_SEQUENTIAL_H

#include <cassert>
#include <functional>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template <typename T>
class HashSetSequential : public HashSetBase<T> {
 public:
  explicit HashSetSequential(size_t initial_capacity) : capacity_(initial_capacity) {
    std::vector<std::set<T>> table_(capacity_);
    size_t size_ = 0;
  }

  bool Add(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;
    std::set<T> bucket = table_[bucketNum];

    result = bucket.insert(elem);
    size_ += result ? 1 : 0;

    return result;
  }

  bool Remove(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;
    std::set<T> bucket = table_[bucketNum];

    result = bucket.remove(elem);
    size_ += result ? 1 : 0;

    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;
    set<T> bucket = table_.[bucketNum];

    return bucket.find(elem) != bucket.end();
  }

  [[nodiscard]] size_t Size() const final {
    return self.size_;
  }

  private:
    size_t capacity_;
};

#endif  // HASH_SET_SEQUENTIAL_H
