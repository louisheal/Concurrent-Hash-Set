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
  explicit HashSetSequential(size_t initial_capacity) : table_(initial_capacity), size_(0) {
  }

  bool Add(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % table_.size();
    bool result = table_[bucketNum].insert(elem).second;
    size_ += result ? 1 : 0;
    if (policy()) {
      resize();
    }
    return result;
  }

  bool Remove(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % table_.size();
    return table_[bucketNum].erase(elem) == 1;
  }

  [[nodiscard]] bool Contains(T elem) final {
    size_t bucketNum = std::hash<T>()(elem) % table_.size();
    return table_[bucketNum].find(elem) != table_.end();
  }

  [[nodiscard]] size_t Size() const final {
    return size_;
  }

 private:

  // Checking whether we need to resize the hashset to guarantee const time operations
  bool policy() {
    return size_ / table_.size() > 4;
  }

  void resize() {
    int old_capacity = table_.size();
    int new_capacity = 2 * old_capacity;
    std::vector<std::set<T>> new_table(new_capacity);
    for (std::set<T> &bucket: table_) {
      for (T &elem: bucket) {
        size_t bucket_num = std::hash<T>()(elem) % new_capacity;
        new_table[bucket_num].insert(elem);
      }
    }
    table_ = new_table;
  }

 private:
  //  Number of elements in the hashset
  size_t size_;
  //  Vector of buckets for elements
  std::vector<std::set<T>> table_;
};

#endif  // HASH_SET_SEQUENTIAL_H
