#ifndef HASH_SET_STRIPED_H
#define HASH_SET_STRIPED_H

#include <atomic>
#include <cassert>
#include <functional>
#include <mutex>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template <typename T> class HashSetStriped : public HashSetBase<T> {
public:
  explicit HashSetStriped(size_t initial_capacity)
      : size_(0u), table_(initial_capacity), locks_(initial_capacity) {}

  bool Add(T elem) final {
    Acquire(elem);
    size_t bucket_num = std::hash<T>()(elem) % table_.size();
    bool result = table_[bucket_num].insert(elem).second;
    if (result) {
      size_.fetch_add(1u);
    }
    // Have to get values of Policy() and table_.size() before we release the
    // lock, so that we know if another thread resizes the table when the
    // current thread attempts to
    bool policy = Policy();
    size_t old_capacity = table_.size();
    Release(elem);
    if (policy) {
      Resize(old_capacity);
    }
    return result;
  }

  bool Remove(T elem) final {
    Acquire(elem);
    size_t hash = std::hash<T>()(elem);
    bool result = table_[hash % table_.size()].erase(elem);
    if (result) {
      size_.fetch_sub(1u);
    }
    Release(elem);
    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    Acquire(elem);
    size_t bucket_num = std::hash<T>()(elem) % table_.size();
    bool result = table_[bucket_num].find(elem) != table_[bucket_num].end();
    Release(elem);
    return result;
  }

  [[nodiscard]] size_t Size() const final { return size_.load(); }

private:
  void Acquire(T elem) {
    size_t hash_code = std::hash<T>()(elem);
    locks_[hash_code % locks_.size()].lock();
  }

  void Release(T elem) {
    size_t hash_code = std::hash<T>()(elem);
    locks_[hash_code % locks_.size()].unlock();
  }

  bool Policy() { return size_.load() / table_.size() > 4; }

  void Resize(size_t old_capacity) {

    for (auto &lock : locks_) {
      lock.lock();
    }

    if (old_capacity != table_.size()) {
      for (auto &lock : locks_) {
        lock.unlock();
      }
      return;
    }

    size_t new_capacity = 2 * old_capacity;

    std::vector<std::set<T>> old_table;
    old_table.assign(table_.begin(), table_.end());

    table_.clear();
    table_.resize(new_capacity);

    for (std::set<T> &bucket : old_table) {
      for (auto &elem : bucket) {
        size_t bucket_num = std::hash<T>()(elem) % new_capacity;
        table_[bucket_num].insert(elem);
      }
    }

    for (auto &lock : locks_) {
      lock.unlock();
    }
  }

private:
  //  Number of elements in the hashset
  std::atomic<size_t> size_;
  //  Vector of buckets for elements
  std::vector<std::set<T>> table_;
  //  Vector of mutexes for each stripe
  std::vector<std::mutex> locks_;
};

#endif // HASH_SET_STRIPED_H
