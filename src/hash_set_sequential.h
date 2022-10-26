#ifndef HASH_SET_SEQUENTIAL_H
#define HASH_SET_SEQUENTIAL_H

#include <cassert>
#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template <typename T> class HashSetSequential : public HashSetBase<T> {
public:
  explicit HashSetSequential(size_t initial_capacity)
      : size_(0u),
        table_(std::make_unique<std::vector<std::set<T>>>(initial_capacity)) {}

  bool Add(T elem) final {
    size_t bucket_num = std::hash<T>()(elem) % (*table_).size();
    bool result = (*table_)[bucket_num].insert(elem).second;
    size_ += result ? 1 : 0;
    if (Policy()) {
      Resize();
    }
    return result;
  }

  bool Remove(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % (*table_).size();
    bool result = (*table_)[bucketNum].erase(elem) == 1;
    size_ -= result ? 1 : 0;
    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % (*table_).size();
    return (*table_)[bucketNum].find(elem) != (*table_)[bucketNum].end();
  }

  [[nodiscard]] size_t Size() const final { return size_; }

private:
  // Checking whether we need to resize the hashset to guarantee
  // constant time operations.
  bool Policy() { return size_ / (*table_).size() > 4; }

  void Resize() {
    size_t old_capacity = (*table_).size();
    size_t new_capacity = 2 * old_capacity;
    auto new_table(std::make_unique<std::vector<std::set<T>>>(new_capacity));
    // Transfer of old elements to the resized table
    for (std::set<T> &bucket : (*table_)) {
      for (auto &elem : bucket) {
        size_t bucket_num = std::hash<T>()(elem) % new_capacity;
        (*new_table)[bucket_num].insert(elem);
      }
    }
    table_->clear();
    table_ = std::move(new_table);
  }

private:
  //  Number of elements in the hashset
  size_t size_;
  //  Vector of buckets for elements
  std::unique_ptr<std::vector<std::set<T>>> table_;
};

#endif // HASH_SET_SEQUENTIAL_H
