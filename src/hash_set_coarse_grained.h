#ifndef HASH_SET_COARSE_GRAINED_H
#define HASH_SET_COARSE_GRAINED_H

#include "src/hash_set_base.h"
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

template <typename T> class HashSetCoarseGrained : public HashSetBase<T> {
public:
  explicit HashSetCoarseGrained(size_t initial_capacity)
      : size_(0u),
        table_(std::make_unique<std::vector<std::set<T>>>(initial_capacity)) {}

  bool Add(T elem) final {
    std::scoped_lock<std::mutex> lock(mutex_);
    size_t bucket_num = std::hash<T>()(elem) % (*table_).size();
    bool result = (*table_)[bucket_num].insert(elem).second;
    size_ += result ? 1 : 0;
    if (Policy()) {
      Resize();
    }
    return result;
  }

  bool Remove(T elem) final {
    std::scoped_lock<std::mutex> lock(mutex_);
    size_t bucket_num = std::hash<T>()(elem) % (*table_).size();
    bool result = (*table_)[bucket_num].erase(elem) == 1;
    size_ -= result ? 1 : 0;
    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    std::scoped_lock<std::mutex> lock(mutex_);
    size_t bucket_num = std::hash<T>()(elem) % (*table_).size();
    return (*table_)[bucket_num].find(elem) != (*table_)[bucket_num].end();
  }

  [[nodiscard]] size_t Size() const final {
    std::scoped_lock<std::mutex> lock(mutex_);
    return size_;
  }

private:
  // Checking whether we need to resize the hashset to guarantee
  // constant time operations
  bool Policy() { return size_ / (*table_).size() > 4; }

  void Resize() {
    size_t old_capacity = (*table_).size();
    size_t new_capacity = 2 * old_capacity;
    // Created as a pointer, so we can move it to table_
    auto new_table(std::make_unique<std::vector<std::set<T>>>(new_capacity));
    // Transfer of old elements to the resized table
    for (std::set<T> &bucket : (*table_)) {
      for (auto &elem : bucket) {
        size_t bucket_num = std::hash<T>()(elem) % new_capacity;
        (*new_table)[bucket_num].insert(elem);
      }
    }
    // Deleting unused table
    table_->clear();
    table_ = std::move(new_table);
  }

private:
  // Number of elements in the hashset
  size_t size_;
  // Vector of buckets for elements. Made as unique_ptr because we need to
  // resize the table, thus creating new object and point to it.
  std::unique_ptr<std::vector<std::set<T>>> table_;
  // Lock is mutable, so it can be locked within Size(), which is const function
  mutable std::mutex mutex_;
};

#endif // HASH_SET_COARSE_GRAINED_H
