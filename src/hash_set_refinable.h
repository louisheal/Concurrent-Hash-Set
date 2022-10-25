#ifndef HASH_SET_REFINABLE_H
#define HASH_SET_REFINABLE_H

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template <typename T> class HashSetRefinable : public HashSetBase<T> {
public:
  explicit HashSetRefinable(size_t initial_capacity)
      : resizing_(false), num_locked_locks_(0), size_(0),
        table_(initial_capacity) {
    for (size_t i = 0; i < initial_capacity; i++) {
      std::unique_ptr<std::mutex> lock = std::make_unique<std::mutex>();
      locks_.push_back(std::move(lock));
    }
  }

  bool Add(T elem) final {
    auto lock = acquire(elem);
    size_t bucket_num = std::hash<T>()(elem) % table_.size();
    bool result = table_[bucket_num].insert(elem).second;
    if (result) {
      size_.fetch_add(1);
    }
    release(std::move(lock));
    if (policy()) {
      resize();
    }
    return result;
  }

  bool Remove(T elem) final {
    auto lock = acquire(elem);
    size_t bucket_num = std::hash<T>()(elem) % table_.size();
    bool result = table_[bucket_num].erase(elem);
    if (result) {
      size_.fetch_sub(1);
    }
    release(std::move(lock));
    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    auto lock = acquire(elem);
    size_t bucket_num = std::hash<T>()(elem) % table_.size();
    bool result = table_[bucket_num].find(elem) != table_[bucket_num].end();
    release(std::move(lock));
    return result;
  }

  [[nodiscard]] size_t Size() const final { return size_.load(); }

private:
  std::unique_ptr<std::unique_lock<std::mutex>> acquire(T elem) {
    size_t lock_num = std::hash<T>()(elem) % locks_.size();
    std::unique_ptr<std::unique_lock<std::mutex>> lock =
        std::make_unique<std::unique_lock<std::mutex>>(*locks_[lock_num]);

    is_resizing_.wait(*lock, [this]() -> bool { return !resizing_.load(); });
    num_locked_locks_.fetch_add(1);
    return lock;
    //        size_t new_lock_num = std::hash<T>()(elem) % locks_.size();
    //        if (lock_num != new_lock_num) {
    //            lock.unlock();
    //            return Contains(elem);
    //        }
  }

  void release(std::unique_ptr<std::unique_lock<std::mutex>> lock) {
    num_locked_locks_.fetch_sub(1);
    lock->unlock();
    lock.release();
  }

  bool policy() { return size_.load() / table_.size() > 4; }

  void wait_all_finish() {
    while (num_locked_locks_.load() != 0) {
    }
  }

  void resize() {

    size_t old_capacity = table_.size();
    // if returns false, then some other thread has already resized the set
    bool expected = false;
    if (resizing_.compare_exchange_strong(expected, true)) {
      if (old_capacity != table_.size()) {
        // Someone has already resized
        resizing_.store(false);
        return;
      }
      wait_all_finish();

      size_t new_capacity = 2 * old_capacity;
      //      locks_.resize(new_capacity);

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
      resizing_.store(false);
      is_resizing_.notify_all();
    }
  }

private:
  std::atomic<bool> resizing_;
  std::atomic<int> num_locked_locks_;
  //  Number of elements in the hashset
  std::atomic<size_t> size_;
  //  Vector of buckets for elements
  std::vector<std::set<T>> table_;

  //  Vector of mutexes for each stripe
  std::vector<std::unique_ptr<std::mutex>> locks_;
  std::condition_variable is_resizing_;
};

#endif // HASH_SET_REFINABLE_H
