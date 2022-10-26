#ifndef HASH_SET_REFINABLE_H
#define HASH_SET_REFINABLE_H

#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <vector>

#include "src/hash_set_base.h"

template <typename T> class HashSetRefinable : public HashSetBase<T> {
public:
  explicit HashSetRefinable(size_t initial_capacity)
      : size_(0), table_(initial_capacity),
        locks_(std::make_unique<std::vector<std::mutex>>(initial_capacity)),
        bucket_count_(initial_capacity) {}

  bool Add(T elem) final {
    // The read-write lock prevents other threads
    // from resizing the table when a thread accesses it
    read_write_lock_.lock_shared();
    size_t lock_num = std::hash<T>()(elem) % bucket_count_.load();
    (*locks_)[lock_num].lock();
    bool result = table_[lock_num].insert(elem).second;
    if (result) {
      size_.fetch_add(1);
    }
    bool policy = Policy();
    size_t old_capacity = bucket_count_.load();
    // Unlocking locks in the reverse order of how they were locked
    // in order to prevent deadlocks
    (*locks_)[lock_num].unlock();
    read_write_lock_.unlock();
    if (policy) {
      Resize(old_capacity);
    }
    return result;
  }

  bool Remove(T elem) final {
    read_write_lock_.lock_shared();
    size_t lock_num = std::hash<T>()(elem) % bucket_count_.load();
    (*locks_)[lock_num].lock();
    bool result = table_[lock_num].erase(elem);
    if (result) {
      size_.fetch_sub(1);
    }
    (*locks_)[lock_num].unlock();
    read_write_lock_.unlock();
    return result;
  }

  [[nodiscard]] bool Contains(T elem) final {
    read_write_lock_.lock_shared();
    size_t lock_num = std::hash<T>()(elem) % bucket_count_.load();
    (*locks_)[lock_num].lock();
    bool result = table_[lock_num].find(elem) != table_[lock_num].end();
    (*locks_)[lock_num].unlock();
    read_write_lock_.unlock();
    return result;
  }

  [[nodiscard]] size_t Size() const final { return size_.load(); }

private:
  bool Policy() { return size_.load() / bucket_count_.load() > 4; }

  void Resize(size_t old_capacity) {

    read_write_lock_.lock();

    if (old_capacity != bucket_count_.load()) {
      read_write_lock_.unlock();
      return;
    }

    size_t new_capacity = 2 * old_capacity;
    bucket_count_.store(new_capacity);
    auto new_locks(std::make_unique<std::vector<std::mutex>>(new_capacity));

    std::vector<std::set<T>> old_table;
    old_table.assign(table_.begin(), table_.end());

    table_.clear();
    table_.resize(new_capacity);

    // Moves the elements of the hash set into a new table
    for (std::set<T> &bucket : old_table) {
      for (auto &elem : bucket) {
        size_t bucket_num = std::hash<T>()(elem) % new_capacity;
        table_[bucket_num].insert(elem);
      }
    }

    locks_->clear();
    locks_ = std::move(new_locks);

    read_write_lock_.unlock();
  }

private:
  //  Number of elements in the hashset
  std::atomic<size_t> size_;
  //  Vector of buckets for elements
  std::vector<std::set<T>> table_;
  //  Vector of mutexes for each stripe
  std::unique_ptr<std::vector<std::mutex>> locks_;
  //  Number of buckets
  std::atomic<size_t> bucket_count_;
  //  Read / Write lock
  std::shared_mutex read_write_lock_;
};

#endif // HASH_SET_REFINABLE_H
