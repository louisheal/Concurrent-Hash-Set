#ifndef HASH_SET_SEQUENTIAL_H
#define HASH_SET_SEQUENTIAL_H

#include <cassert>
#include <functional>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template<typename T>
class HashSetSequential : public HashSetBase<T> {
public:
  explicit HashSetSequential(size_t initial_capacity) : capacity_(initial_capacity) {
    table_.resize(capacity_);
    size_ = 0;
  }

  bool Add(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;
    auto result = table_[bucketNum].insert(elem).second;

    size_ += result ? 1 : 0;
    return result;
  }

  bool Remove(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;
    bool result = table_[bucketNum].erase(elem);

    size_ -= result ? 1 : 0;
    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % capacity_;
    return table_[bucketNum].count(elem) == 1;
  }

  [[nodiscard]] size_t Size() const final {
    return size_;
  }

  private:
    size_t capacity_;
    size_t size_;
    std::vector<std::set<T>> table_;
};

#endif  // HASH_SET_SEQUENTIAL_H
