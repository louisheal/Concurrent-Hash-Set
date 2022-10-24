#ifndef HASH_SET_COARSE_GRAINED_H
#define HASH_SET_COARSE_GRAINED_H

#include <cassert>
#include <functional>
#include <mutex>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template<typename T>
class HashSetCoarseGrained : public HashSetBase<T> {
  public:
  explicit HashSetCoarseGrained(size_t initial_capacity) : size_(0), table_(initial_capacity) {
  }

  bool Add(T elem) final {
    std::scoped_lock<std::mutex> lock(mutex_);
    size_t bucketNum = std::hash<T>()(elem) % table_.size();
    auto result = table_[bucketNum].insert(elem).second;
    size_ += result ? 1 : 0;
    if (policy()) {
      resize();
    }
    return result;
  }

  bool Remove(T elem) final {
    std::scoped_lock<std::mutex> lock(mutex_);
    size_t bucketNum = std::hash<T>()(elem) % table_.size();
    bool result = table_[bucketNum].erase(elem);
    size_ -= result ? 1 : 0;
    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    std::scoped_lock<std::mutex> lock(mutex_);
    size_t bucketNum = std::hash<T>()(elem) % table_.size();
    return table_[bucketNum].count(elem) == 1;
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
      size_t old_capacity = table_.size();

      if (old_capacity != table_.size()) {
        return;
      }

      size_t new_capacity = 2 * old_capacity;

      std::vector<std::set<int>> old_table;
      old_table.assign(table_.begin(), table_.end());

      table_.clear(); table_.resize(new_capacity);

      for (std::set<int> &bucket: old_table) {
        for (int elem: bucket) {
          size_t bucket_num = std::hash<int>()(elem) % new_capacity;
          table_[bucket_num].insert(elem);
        }
      }
    }

  private:
    size_t size_;
    std::vector<std::set<T>> table_;
    std::mutex mutex_;
};

#endif  // HASH_SET_COARSE_GRAINED_H
